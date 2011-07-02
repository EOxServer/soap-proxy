#-------------------------------------------------------------------------------
# $Id$
#
# Project: EOxServer <http://eoxserver.org>
# Authors: Stephan Krause <stephan.krause@eox.at>
#          Stephan Meissl <stephan.meissl@eox.at>
#
#-------------------------------------------------------------------------------
# Copyright (C) 2011 EOX IT Services GmbH
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
# copies of the Software, and to permit persons to whom the Software is 
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies of this Software or works derived from this Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#-------------------------------------------------------------------------------

"""
This module contains the abstract base classes for request handling.
"""

from email.parser import Parser as MIMEParser
from email.message import Message
import logging
import os.path
from cgi import escape
from osgeo.gdalconst import GDT_Byte, GDT_Int16, GDT_UInt16, GDT_Float32

from django.conf import settings

from eoxserver.resources.coverages.domainset import Trim, Slice
from eoxserver.services.base import BaseRequestHandler
from eoxserver.services.owscommon import OWSCommonConfigReader
from eoxserver.services.requests import OWSRequest, Response
from eoxserver.services.exceptions import (
    InvalidRequestException, VersionNegotiationException
)
from eoxserver.contrib import mapscript

class MapServerRequest(OWSRequest):
    def __init__(self, req):
        super(MapServerRequest, self).__init__(req.http_req, params=req.params, decoder=req.decoder)
        
        self.version = req.version
        
        self.map = mapscript.mapObj()
        self.ows_req = mapscript.OWSRequest()

class MapServerResponse(Response):
    def __init__(self, ms_response, ms_content_type, ms_status, headers={}, status=None):
        super(MapServerResponse, self).__init__(content=ms_response, content_type=ms_content_type, headers=headers, status=status)
        self.ms_status = ms_status
        
        self.ms_response_data = None
        self.ms_response_xml = ''
        self.ms_response_xml_headers = {}
        
    def _isXML(self, content_type):
        return content_type.split(";")[0].lower() in ("text/xml", "application/xml")
            
    def _isMultipart(self, content_type):
        return content_type.split("/")[0].lower() == "multipart"
        
    def splitResponse(self):
        if self._isXML(self.content_type):
            self.ms_response_xml = self.content
            self.ms_response_xml_headers = {'Content-type': self.content_type}
        elif self._isMultipart(self.content_type):
            parts = MIMEParser().parsestr("Content-type:%s\n\n%s" % (self.content_type, self.content.rstrip("--wcs--\n\n"))).get_payload()
            for part in parts:
                if self._isXML(part.get_content_type()):
                    self.ms_response_xml = part.get_payload()
                    for header in part.keys():
                        self.ms_response_xml_headers[header] = part[header]
                else:
                    self.ms_response_data = part
        else:
            self.ms_response_data = self.content
    
    def getProcessedResponse(self, processed_xml, processed_xml_headers=None):
        if self.ms_response_data is not None:
            xml_msg = Message()
            xml_msg.set_payload(processed_xml)
            
            if processed_xml_headers is not None:
                xml_headers = processed_xml_headers
            else:
                xml_headers = self.ms_response_xml_headers
            for name, value in xml_headers.items():
                xml_msg.add_header(name, value)
            
            if isinstance(self.ms_response_data, Message):
                data_msg = self.ms_response_data
            else:
                data_msg = Message()
                
                data_msg.set_payload(self.ms_response_data)
                
                data_msg.add_header('Content-type', self.content_type)
                for name, value in self.headers.items():
                    data_msg.add_header(name, value)
                
            content = "--wcs\n%s\n--wcs\n%s\n--wcs--" % (xml_msg.as_string(), data_msg.as_string())
            content_type = "multipart/mixed; boundary=wcs"
            headers = {}
        else:
            content = processed_xml
            content_type = self.content_type
            if processed_xml_headers is None:
                headers = self.headers
            else:
                headers = processed_xml_headers
            
        return Response(content, content_type, headers, self.getStatus())
        
    def getContentType(self):
        if "Content-type" in self.headers:
            return self.headers["Content-type"]
        else:
            return self.content_type # TODO: headers for multipart messages
        
    def getStatus(self):
        if self.status is None:
            if self.ms_status is not None and self.ms_status == 0:
                return 200
            else:
                return 400
        else:
            return self.status

        
class MapServerOperationHandler(BaseRequestHandler):
    """\
    MapServerOperationHandler serves as parent class for all operations
    involving calls to MapServer. It is not an abstract class, but implements
    the most basic functionality, i.e. simply passing on a request to
    MapServer as it comes in.


    This class implements a workflow for request handling that
    involves calls to MapServer using its Python binding (mapscript).
    Requests are processed in six steps:
    
    <ol>
    <li>retrieve coverage information ({@link #createCoverages} method)</li>
    <li>configure a MapServer <tt>OWSRequest</tt> object with
        parameters from the request ({@link #configureRequest}
        method)</li>
    <li>configure a MapServer <tt>mapObj</tt> object with
        parameters from the request and the config
        ({@link #configureMapObj} method)</li>
    <li>add layers to the MapServer <tt>mapObj</tt>
        ({@link #addLayers} method)</li>
    <li>dispatch the request, i.e. let MapServer actually do its
        work; return the result ({@link dispatch} method)</li>
    <li>postprocess the MapServer response ({@link postprocess}
        method)</li>
    </ol>
    
    Child classes may override the configureRequest, configureMap,
    postprocess and postprocessFault methods in order to customize
    functionality. If possible, the handle and dispatch methods should
    not be overridden.
    """
    
    PARAM_SCHEMA = {
        "service": {"xml_location": "/@service", "xml_type": "string", "kvp_key": "service", "kvp_type": "string"},
        "version": {"xml_location": "/@version", "xml_type": "string", "kvp_key": "version", "kvp_type": "string"},
        "operation": {"xml_location": "/", "xml_type": "localName", "kvp_key": "request", "kvp_type": "string"}
    }
    
    def _processRequest(self, req):
        """
        This method implements the workflow described in the class
        documentation.
        
        First it creates a MapServerRequest object and passes the
        request data to it. Then it invokes the methods in the order
        defined above and finally returns an {@link eoxserver.lib.requests.MapServerResponse}
        object. It is not recommended to override this method.
        
        @param  req An {@link eoxserver.lib.requests.EOxSOWSRequest}
                    object containing the request parameters and data
        
        @return     An {@link eoxserver.lib.requests.EOxSMapServerResponse}
                    object containing the response content, headers and
                    status as well as the status code returned by
                    MapServer
        """
        ms_req = MapServerRequest(req)
        ms_req.setSchema(self.PARAM_SCHEMA)
        self.createCoverages(ms_req)
        self.configureRequest(ms_req)
        self.configureMapObj(ms_req)
        self.addLayers(ms_req)
        response = self.dispatch(ms_req)
        return self.postprocess(ms_req, response)
    
    def createCoverages(self, ms_req):
        """
        This method creates coverages, i.e. it adds coverage objects to
        the <tt>ms_req.coverages</tt> list. The default implementation
        does nothing at all, so you will have to override this method to
        meet your needs. 
        
        @param  ms_req  An {@link eoxserver.lib.requests.EOxSMapServerRequest} object
        
        @return         None
        """
        pass
    
    def _setParameter(self, ms_req, key, value):
        ms_req.ows_req.setParameter(key, value)

    def configureRequest(self, ms_req):
        """
        This method configures the <tt>ms_req.ows_req</tt> object (an
        instance of <tt>mapscript.OWSRequest</tt>) with the parameters
        passed in with the user request. This method can be overridden
        in order to change the treatment of parameters.
        
        @param  ms_req  An {@link eoxserver.lib.requests.EOxSMapServerRequest} object
        
        @return         None
        """
        
        ms_req.ows_req = mapscript.OWSRequest()
        if ms_req.getParamType() == "kvp":
            for key, values in ms_req.getParams().items():
                try: # first try 'addParameter'
                    for value in values:
                        ms_req.ows_req.addParameter(key, value)
                except AttributeError:
                    if len(values) == 1:
                        self._setParameter(ms_req, key.lower(), escape(values[0]))
                    else:
                        c = 0
                        for value in values:
                            if c == 0:
                                new_key = key
                            else:
                                new_key = "%s_%d" % (key.lower(), c)
                            self._setParameter(ms_req, new_key, escape(value))
                            
                            c += 1
                            while "%s_%d" % (key.lower(), c) in ms_req.getParams():
                                c += 1

            self._setParameter(ms_req, "version", ms_req.getVersion())
        elif ms_req.getParamType() == "xml":
            ms_req.ows_req.type = mapscript.MS_POST_REQUEST
            ms_req.ows_req.postrequest = ms_req.getParams()
        else:
            raise Exception("Unknown parameter type '%s'." % ms_req.getParamType())

    def configureMapObj(self, ms_req):
        """
        This method configures the <tt>ms_req.map</tt> object (an
        instance of <tt>mapscript.mapObj</tt>) with parameters from the
        config. This method can be overridden in order to implement more
        sophisticated behaviour. 
        
        @param  ms_req  An {@link eoxserver.lib.requests.EOxSMapServerRequest} object
        
        @return         None
        """
        ms_req.map = mapscript.mapObj(os.path.join(settings.PROJECT_DIR, "conf", "template.map")) # TODO: Store information in database??? (title, abstract, etc.)
        #ms_req.map.loadOWSParameters(ms_req.ows_req)
        
        ms_req.map.setMetaData("ows_onlineresource", OWSCommonConfigReader().getHTTPServiceURL() + "?")
        ms_req.map.setMetaData("wcs_label", "EOxServer WCS") # TODO: to WCS
        
        #ms_req.map.debug = 5
        #ms_req.map.setConfigOption("MS_ERRORFILE", "stderr")
        
    def addLayers(self, ms_req):
        """
        This method adds layers to the <tt>ms_req.map</tt> object based
        on the coverages defined in <tt>ms_req.coverages</tt>. The
        default is to unconditionally add a single layer for each
        coverage defined. This method can be overridden in order to
        customize the way layers are inserted into the map object.
        
        @param  ms_req  An {@link eoxserver.lib.requests.EOxSMapServerRequest} object
        
        @return         None
        """
        for coverage in ms_req.coverages:
            ms_req.map.insertLayer(self.getMapServerLayer(coverage))
    
    def getMapServerLayer(self, coverage, **kwargs):
        """
        This method is invoked by the {@link #addLayers} method in order
        to generate <tt>mapscript.layerObj</tt> instances for each
        coverage. The basic configuration is done here, but subclasses
        will have to override this method in order to define e.g. the
        data sources for the layer.
        
        @param  coverage    An {@link eoxserver.lib.interfaces.EOxSCoverageInterface}
                            object giving access to the coverage data
        
        @return             A <tt>mapscribt.layerObj</tt> object
                            representing the layer to be inserted
        """
        layer = mapscript.layerObj()
        
        layer.name = coverage.getCoverageId()
        layer.status = mapscript.MS_ON

        # TODO: getSRID is defined for rectified coverages only
        layer.setProjection("+init=epsg:%d" % int(coverage.getSRID()))

        for key, value in coverage.getLayerMetadata():
            layer.setMetaData(key, value)

        return layer


    def dispatch(self, ms_req):
        """
        This method actually executes the MapServer request by calling
        <tt>ms_req.map.OWSDispatch()</tt>. This method should not be
        overridden by child classes.
        
        @param  ms_req  An {@link eoxserver.lib.requests.EOxSMapServerRequest}
                        object
        
        @return         An {@link eoxserver.lib.requests.EOxSMapServerResponse}
                        object containing the content, headers and status
                        of the request as well as the status code
                        returned by MapServer
        """
        logging.debug("MapServerOperationHandler.dispatch: 1")
        mapscript.msIO_installStdoutToBuffer()
        # Execute the OWS request by mapserver, obtain the status in dispatch_status (==0 is OK)
        logging.debug("MapServerOperationHandler.dispatch: 2")
        dispatch_status = ms_req.map.OWSDispatch(ms_req.ows_req)
        
        logging.debug("MapServerOperationHandler.dispatch: 3")
        content_type = mapscript.msIO_stripStdoutBufferContentType()
        mapscript.msIO_stripStdoutBufferContentHeaders()
        logging.debug("MapServerOperationHandler.dispatch: 4")
        result = mapscript.msIO_getStdoutBufferBytes()
        logging.debug("MapServerOperationHandler.dispatch: 5")
        mapscript.msCleanup()
        
        return MapServerResponse(result, content_type, dispatch_status)
        
    def postprocess(self, ms_req, resp):
        """
        This method operates on the MapServer response. The default
        behaviour is to do nothing at all, i.e. return the input
        response unchanged. If postprocessing is needed, you should
        override this method. 
        
        @param  ms_req  An {@link eoxserver.lib.requests.EOxSMapServerRequest}
                        object
        @param  resp    An {@link eoxserver.lib.requests.EOxSMapServerResponse}
                        object.
        """
        return resp
        
    def postprocessFault(self, ms_req, resp):
        return resp

def gdalconst_to_imagemode(const):
    if const == GDT_Byte:
        return mapscript.MS_IMAGEMODE_BYTE
    elif const == GDT_Int16 or const == GDT_UInt16:
        return mapscript.MS_IMAGEMODE_INT16
    elif const == GDT_Float32:
        return mapscript.MS_IMAGEMODE_FLOAT32
    else:
        raise InternalError("MapServer is not capable to process the datatype %d" % rangetype.data_type)

def gdalconst_to_imagemode_string(const):
    if const == GDT_Byte:
        return "BYTE"
    elif const == GDT_Int16 or const == GDT_UInt16:
        return "INT16"
    elif const == GDT_Float32:
        return "FLOAT32"
    
