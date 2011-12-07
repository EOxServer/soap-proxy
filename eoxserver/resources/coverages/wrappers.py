#-----------------------------------------------------------------------
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
#-----------------------------------------------------------------------

"""
This module provides implementations of coverage interfaces as 
defined in :mod:`eoxserver.resources.coverages.interfaces`. These
classes wrap the resources stored in the database and augment them
with additional application logic.
"""

import os.path

import logging

from django.contrib.gis.geos import GEOSGeometry

from eoxserver.core.system import System
from eoxserver.core.resources import (
    ResourceFactoryInterface, ResourceWrapper, ResourceFactory
)
from eoxserver.core.exceptions import (
    InternalError, InvalidParameterException
)
from eoxserver.core.util.filetools import findFiles
from eoxserver.core.util.timetools import getDateTime
from eoxserver.resources.coverages.models import (
    PlainCoverageRecord, RectifiedDatasetRecord,
    ReferenceableDatasetRecord, RectifiedStitchedMosaicRecord,
    DatasetSeriesRecord, EOMetadataRecord, 
    ExtentRecord, LineageRecord, RangeTypeRecord,
    LayerMetadataRecord
)
from eoxserver.resources.coverages.interfaces import (
    CoverageInterface, RectifiedDatasetInterface,
    ReferenceableDatasetInterface, RectifiedStitchedMosaicInterface, 
    DatasetSeriesInterface
)
from eoxserver.resources.coverages.rangetype import (
    Band, NilValue, RangeType
)

#-----------------------------------------------------------------------
# Wrapper implementations
#-----------------------------------------------------------------------

class CoverageWrapper(ResourceWrapper):
    """
    This is the base class for all coverage wrappers. It is a partial
    implementation of :class:`~.CoverageInterface`. It inherits from
    :class:`~.ResourceWrapper`.
    """
    
    #-------------------------------------------------------------------
    # CoverageInterface methods
    #-------------------------------------------------------------------
    
    @property
    def __model(self):
        return self._ResourceWrapper__model
    
    def getCoverageId(self):
        """
        Returns the Coverage ID.
        """
        
        return self.__model.coverage_id
    
    def getCoverageSubtype(self):
        """
        This method shall return the coverage subtype as defined in
        the WCS 2.0 EO-AP. It must be overridden by concrete
        coverage wrappers. By default this method raises
        :exc:`~.InternalError`.
        
        See the definition of
        :meth:`~.CoverageInterface.getCoverageSubtype` in
        :class:`~.CoverageInterface` for possible return values.
        """
        
        raise InternalError("Not implemented.")

    def getType(self):
        """
        This method shall return the internal coverage type code. It
        must be overridden by concrete coverage wrappers. By default
        this method raises :exc:`~.InternalError`.
        
        See the definition of :meth:`~.CoverageInterface.getType` in
        :class:`~.CoverageInterface` for possible return values.
        """
        
        raise InternalError("Not implemented.")
        
    def getSize(self):
        """
        This method shall return a tuple ``(xsize, ysize)`` for the
        coverage wrapped by the implementation. It has to be overridden
        by concrete coverage wrappers. By default this method raises
        :exc:`~.InternalError`.
        """
        
        raise InternalError("Not implemented.")
    
    def getRangeType(self):
        """
        This method returns the range type of the coverage as 
        :class:`~.RangeType` object.
        """
        
        range_type = RangeType(
            name=self.__model.range_type.name,
            data_type=self.__model.range_type.data_type
        )
        
        for band_record in self.__model.range_type.bands.order_by("rangetype2band__no"):
            nil_values = [
                NilValue(value=nv.value, reason=nv.reason)\
                for nv in band_record.nil_values.all()
            ]
            
            range_type.addBand(Band(
                name=band_record.name,
                identifier=band_record.identifier,
                definition=band_record.definition,
                description=band_record.description,
                uom=band_record.uom,
                nil_values=nil_values,
                gdal_interpretation=band_record.gdal_interpretation
            ))
        
        return range_type
    
    def getDataStructureType(self):
        """
        Returns the data structure type of the coverage. To be implemented
        by subclasses, raises :exc:`~.InternalError` by default.
        """
        raise InternalError("Not implemented.")
        
    def getData(self):
        """
        Returns the a :class:`~.CoverageDataWrapper` object that wraps the
        coverage data, raises :exc:`~.InternalError` by default.
        """
        raise InternalError("Not implemented.")
    
    def getLayerMetadata(self):
        """
        Returns a list of ``(metadata_key, metadata_value)`` pairs
        that represent MapServer metadata tags to be attached to
        MapServer layers.
        """
        
        return self.__model.layer_metadata.values_list("key", "value")
    
    def matches(self, filter_exprs):
        """
        Returns True if the Coverage matches the given filter
        expressions and False otherwise.
        """
        
        for filter_expr in filter_exprs:
            filter = System.getRegistry().findAndBind(
                intf_id = "core.filters.Filter",
                params = {
                    "core.filters.res_class_id": self.__class__.__get_impl_id__(),
                    "core.filters.expr_class_id": filter_expr.__class__.__get_impl_id__(),
                }
            )
            
            if not filter.resourceMatches(filter_expr, self):
                return False
    
        return True
    
    def _get_create_dict(self, params):
        create_dict = super(CoverageWrapper, self)._get_create_dict(params)
        
        if "coverage_id" not in params:
            raise InternalError(
                "Missing mandatory 'coverage_id' parameter for RectifiedDataset creation."
            )
        
        if "range_type_name" not in params:
            raise InternalError(
                "Missing mandatory 'range_type_name' parameter for RectifiedDataset creation."
            )
        
        create_dict["coverage_id"] = params["coverage_id"]
        
        try:
            create_dict["range_type"] = RangeTypeRecord.objects.get(
                name=params["range_type_name"]
            )
        except RangeTypeRecord.DoesNotExist:
            raise InternalError(
                "Unknown range type name '%s'" % params["range_type_name"]
            )
            
        if "data_source" in params and params["data_source"] is not None:
            create_dict["automatic"] = params.get("automatic", True)
            create_dict["data_source"] = params["data_source"].getRecord()
        else:
            create_dict["automatic"] = False
            create_dict["data_source"] = None
        
        layer_metadata = params.get("layer_metadata")

        if layer_metadata:
            create_dict["layer_metadata"] = []
            
            for key, value in layer_metadata.items():
                create_dict["layer_metadata"].append(
                    LayerMetadataRecord.objects.get_or_create(
                        key=key, value=value
                    )[0]
                )
                
        return create_dict

class RectifiedGridWrapper(object):
    """
    This wrapper is intended as a mix-in for coverages that rely on a
    rectified grid. It implements :class:`~.RectifiedGridInterface`.
    """    
    @property
    def __model(self):
        return self._ResourceWrapper__model
        
    def _get_create_dict(self, params):
        create_dict = super(RectifiedGridWrapper, self)._get_create_dict(params)
        
        if "geo_metadata" not in params:
            raise InternalError(
                "Missing mandatory 'coverage_id' parameter for RectifiedDataset creation."
            )
        
        geo_metadata = params["geo_metadata"]
        
        create_dict["extent"] = ExtentRecord.objects.create(
            srid = geo_metadata.srid,
            size_x = geo_metadata.size_x,
            size_y = geo_metadata.size_y,
            minx = geo_metadata.extent[0],
            miny = geo_metadata.extent[1],
            maxx = geo_metadata.extent[2],
            maxy = geo_metadata.extent[3]
        )
        
        return create_dict

    def getSRID(self):
        """
        Returns the SRID of the coverage CRS.
        """
        
        return self.__model.extent.srid
    
    def getExtent(self):
        """
        Returns the coverage extent as a 4-tuple of floating point
        coordinates ``(minx, miny, maxx, maxy)`` expressed in the
        coverage CRS as defined by the SRID returned by :meth:`getSRID`.
        """
        
        return (
            self.__model.extent.minx,
            self.__model.extent.miny,
            self.__model.extent.maxx,
            self.__model.extent.maxy
        )

class ReferenceableGridWrapper(object):
    """
    This wrapper is intended as a mix-in for coverages that rely on
    referenceable grids. It has yet to be implemented.
    
    .. note:: The design for referenceable grids is yet TBD.
    """
    
    pass

class PackagedDataWrapper(object):
    """
    This wrapper is intended as a mix-in for coverages that are stored as
    data packages.
    """    
    @property
    def __model(self):
        return self._ResourceWrapper__model
    
    # TODO: replace by appropriate data package implementation
    
    #def _createFileRecord(self, file_info):
        #return FileRecord.objects.create(
            #path = file_info.filename,
            #metadata_path = file_info.md_filename,
            #metadata_format = file_info.md_format
        #)
    
    #def _updateFileRecord(self, file_info):
        #self.__model.file.path = file_info.filename
        #self.__model.file.metadata_path = file_info.md_filename
        #self.__model.file.metadata_format = file_info.md_format
    
    def getDataStructureType(self):
        """
        Returns the data structure type of the underlying data package
        """
        # TODO: this implementation is inefficient as the data package wrapper
        # is discarded and cannot be reused, thus forcing a second database hit
        # when accessing the actual data
        return self.getData().getDataStructureType()
    
    def getData(self):
        """
        Return the data package wrapper associated with the coverage, i.e. an
        instance of a subclass of :class:`~.DataPackageWrapper`.
        """
        return System.getRegistry().getFromFactory(
            factory_id = "resources.coverages.data.DataPackageFactory",
            params = {
                "record": self.__model.data_package
            }
        )

class TiledDataWrapper(object):
    """
    This wrapper is intended as a mix-in for coverages that are stored in tile
    indices.
    """
    @property
    def __model(self):
        return self._ResourceWrapper__model
        
    def getDataStructureType(self):
        """
        Returns ``"index"``.
        """
        # this is a shortcut; it has to be overridden if any time different
        # data structure types for tiled data should be implemented
        return "index"
    
    def getData(self):
        """
        Returns a :class:`TileIndexWrapper` instance.
        """
        return System.getRegistry().getFromFactory(
            factory_id = "resources.coverages.data.TileIndexFactory",
            params = {
                "record": self.__model.tile_index
            }
        )

class EOMetadataWrapper(object):
    """
    This wrapper class is intended as a mix-in for EO coverages and 
    dataset series as defined in the WCS 2.0 EO-AP.
    """    
    @property
    def __model(self):
        return self._ResourceWrapper__model
    
    def _createEOMetadataRecord(self, file_info):
        return EOMetadataRecord.objects.create(
            timestamp_begin = file_info.timestamp_begin,
            timestamp_end = file_info.timestamp_end,
            footprint = GEOSGeometry(file_info.footprint_wkt),
            eo_gml = file_info.md_xml_text
        )
    
    def _updateEOMetadataRecord(self, file_info):
        self.__model.eo_metadata.timestamp_begin = \
            file_info.timestamp_begin
        self.__model.eo_metadata.timestamp_end = \
            file_info.timestamp_end
        self.__model.eo_metadata.footprint = \
            GEOSGeometry(file_info.footprint_wkt)
        self.__model.eo_metadata.eo_gml = file_info.md_xml_text
    
    def _get_create_dict(self, params):
        create_dict = super(EOMetadataWrapper, self)._get_create_dict(params)
        
        if "eo_metadata" not in params:
            raise InternalError(
                "Missing mandatory 'eo_metadata' parameter for RectifiedDataset creation."
            )
        
        eo_metadata = params["eo_metadata"]
        
        create_dict["eo_id"] = eo_metadata.getEOID()
        
        md_format = eo_metadata.getMetadataFormat()
        if md_format and md_format.getName() == "eogml":
            eo_gml = eo_metadata.getRawMetadata()
        else:
            eo_gml = ""
        
        create_dict["eo_metadata"] = EOMetadataRecord.objects.create(
            timestamp_begin = eo_metadata.getBeginTime(),
            timestamp_end = eo_metadata.getEndTime(),
            footprint = eo_metadata.getFootprint(),
            eo_gml = eo_gml
        )
        
        return create_dict
    
    def getEOID(self):
        """
        Returns the EO ID of the object.
        """
        
        return self.__model.eo_id

    def getBeginTime(self):
        """
        Returns the acquisition begin time as :class:`datetime.datetime`
        object.
        """
        
        return self.__model.eo_metadata.timestamp_begin
    
    def getEndTime(self):
        """
        Returns the acquisition end time as :class:`datetime.datetime`
        object.
        """
        
        return self.__model.eo_metadata.timestamp_end
    
    def getFootprint(self):
        """
        Returns the acquisition footprint as
        :class:`django.contrib.gis.geos.GEOSGeometry` object in
        the EPSG:4326 CRS.
        """
        
        return self.__model.eo_metadata.footprint
        
    def getWGS84Extent(self):
        """
        Returns the WGS 84 extent as 4-tuple of floating point
        coordinates ``(minlon, minlat, maxlon, maxlat)``.
        """
        
        return self.__model.eo_metadata.footprint.extent
    
    def getEOGML(self):
        """
        Returns the EO O&M XML text stored in the metadata.
        """
        
        return self.__model.eo_metadata.eo_gml

class EOCoverageWrapper(EOMetadataWrapper, CoverageWrapper):
    """
    This is a partial implementation of :class:`~.EOCoverageInterface`.
    It inherits from :class:`CoverageWrapper` and
    :class:`EOMetadataWrapper`.
    """
    
    def _get_create_dict(self, params):
        create_dict = super(EOCoverageWrapper, self)._get_create_dict(params)
        
        create_dict["lineage"] = LineageRecord.objects.create()
        
        return create_dict
        
    
    def getEOCoverageSubtype(self):
        """
        This method shall return the EO Coverage subtype according to
        the WCS 2.0 EO-AP. It must be overridden by child
        implementations. By default :exc:`~.InternalError` is raised.
        """
        
        raise InternalError("Not implemented.")
    
    def getDatasets(self, filter_exprs=None):
        """
        This method shall return the datasets associated with this
        coverage, possibly filtered by the optional filter expressions.
        It must be overridden by child implementations. By default
        :exc:`~.InternalError` is raised.
        """
        
        raise InternalError("Not implemented.")
    
    def getLineage(self):
        """
        Returns ``None``.
        
        .. note:: The lineage element has yet to be specified in
                  detail in the WCS 2.0 EO-AP.
        """
        
        return None

class EODatasetWrapper(PackagedDataWrapper, EOCoverageWrapper):
    """
    This is the base class for EO Dataset wrapper implementations. It
    inherits from :class:`EOCoverageWrapper` and
    :class:`PackagedDataWrapper`.
    """
    
    def _get_create_dict(self, params):
        create_dict = super(EODatasetWrapper, self)._get_create_dict(params)
        
        if "data_package" not in params:
            raise InternalError(
                "Missing mandatory 'data_package' parameter for RectifiedDataset creation."
            )
        
        create_dict["data_package"] = params["data_package"].getRecord()
        
        if "container" in params:
            create_dict["visible"] = params.get("visible", False)
        else:
            create_dict["visible"] = params.get("visible", True)
        
        return create_dict
    
    def getDatasets(self, filter_exprs=None):
        """
        This method applies the given filter expressions to the
        model and returns a list containing the wrapper in case the
        filters are matched or an empty list otherwise.
        """
        
        if filter_exprs is not None:
            if not self.matches(filter_exprs):
                return []
        
        return [self]

class RectifiedDatasetWrapper(RectifiedGridWrapper, EODatasetWrapper):
    """
    This is the wrapper for Rectified Datasets. It inherits from
    :class:`EODatasetWrapper` and :class:`RectifiedGridWrapper`. It
    implements :class:`~.RectifiedDatasetInterface`.
    
    The following attributes are recognized:
    
    * ``eo_id``: the EO ID of the dataset; value must be a string
    * ``srid``: the SRID of the dataset's CRS; value must be an integer
    * ``size_x``: the width of the coverage in pixels; value must be
      an integer
    * ``size_y``: the height of the coverage in pixels; value must be
      an integer
    * ``minx``: the left hand bound of the dataset's extent; value must
      be numeric
    * ``miny``: the lower bound of the dataset's extent; value must be
      numeric
    * ``maxx``: the right hand bound of the dataset's extent; value must
      be numeric
    * ``maxy``: the upper bound of the dataset's extent; value must be
      numeric
    * ``visible``: the ``visible`` attribute of the dataset; value must
      be boolean
    * ``automatic``: the ``automatic`` attribute of the dataset; value
      must be boolean
    """
    
    REGISTRY_CONF = {
        "name": "Rectified Dataset Wrapper",
        "impl_id": "resources.coverages.wrappers.RectifiedDatasetWrapper",
        "model_class": RectifiedDatasetRecord,
        "id_field": "coverage_id",
        "factory_ids": ("resources.coverages.wrappers.EOCoverageFactory",)
    }
    
    FIELDS = {
        "eo_id": "eo_id",
        "data_package": "data_package",
        "srid": "extent__srid",
        "size_x": "extent__size_x",
        "size_y": "extent__size_y",
        "minx": "extent__minx",
        "miny": "extent__miny",
        "maxx": "extent__maxx",
        "maxy": "extent__maxy",
        "visible": "visible",
        "automatic": "automatic"
    }
    
    #-------------------------------------------------------------------
    # ResourceInterface implementations
    #-------------------------------------------------------------------
    
    # NOTE: partially implemented in ResourceWrapper
    
    def __get_model(self):
        return self._ResourceWrapper__model
    
    def __set_model(self, model):
        self._ResourceWrapper__model = model
        
    __model = property(__get_model, __set_model)

    # _get_create_dict inherited from superclasses and mix-ins

    def _create_model(self, create_dict):
        self.__model = RectifiedDatasetRecord.objects.create(**create_dict)
        
    def _post_create(self, params):
        if "container" in params and params["container"]:
            params["container"].addCoverage(
                res_type = self.getType(),
                res_id = self.__model.pk
            )
        containers = params.get("containers", [])
        for container in containers:
            container.addCoverage(
                res_type = self.getType(),
                res_id = self.__model.pk
            )
        
    # ==========================================================================
    # TODO: update this part according to new data model

    def _updateModel(self, params):
        file_info = params.get("file_info")
        automatic = params.get("automatic")
        visible = params.get("visible")
        add_container = params.get("add_container")
        rm_container = params.get("rm_container")

        if file_info is not None:

            self._updateFileRecord(file_info)

            self._updateEOMetadataRecord(file_info)
                
            self._updateExtentRecord(file_info)
            
            self._updateLineageRecord(file_info)
            
            range_type_record = RangeTypeRecord.objects.get(
                name = file_info.range_type
            )

            self.__model.coverage_id = file_info.eo_id
            self.__model.eo_id = file_info.eo_id
            self.__model.range_type = range_type_record
        
        if automatic is not None:
            self.__model.automatic = automatic
        
        if visible is not None:
            self.__model.visible = visible
        
        if add_container is not None:
            add_container.addCoverage(self.getType(), self.__model.pk)
        
        if rm_container is not None:
            rm_container.removeCoverage(self.getType(), self.__model.pk)

        # commit update 
        self.__model.save()

    
    def _getAttrValue(self, attr_name):
        if attr_name == "eo_id":
            return self.__model.eo_id
        elif attr_name == "data_package":
            return self.__model.data_package
        elif attr_name == "srid":
            return self.__model.extent.srid
        elif attr_name == "size_x":
            return self.__model.extent.size_x
        elif attr_name == "size_y":
            return self.__model.extent.size_y
        elif attr_name == "minx":
            return self.__model.extent.minx
        elif attr_name == "miny":
            return self.__model.extent.miny
        elif attr_name == "maxx":
            return self.__model.extent.maxx
        elif attr_name == "maxy":
            return self.__model.extent.maxy
        elif attr_name == "visible":
            return self.__model.visible
        elif attr_name == "automatic":
            return self.__model.automatic
    
    def _setAttrValue(self, attr_name, value):
        if attr_name == "eo_id":
            self.__model.eo_id = value
        elif attr_name == "data_package":
            self.__model.data_package = value
        elif attr_name == "srid":
            self.__model.extent.srid = value
        elif attr_name == "size_x":
            self.__model.extent.size_x = value
        elif attr_name == "size_y":
            self.__model.extent.size_y = value
        elif attr_name == "minx":
            self.__model.extent.minx = value
        elif attr_name == "miny":
            self.__model.extent.miny = value
        elif attr_name == "maxx":
            self.__model.extent.maxx = value
        elif attr_name == "maxy":
            self.__model.extent.maxy = value
        elif attr_name == "visible":
            self.__model.visible = value
        elif attr_name == "automatic":
            self.__model.automatic = value

    # END TODO
    #===========================================================================
    
    #-------------------------------------------------------------------
    # CoverageInterface implementations
    #-------------------------------------------------------------------
    
    # NOTE: partially implemented in CoverageWrapper
    
    def getCoverageSubtype(self):
        """
        Returns ``RectifiedGridCoverage``.
        """
        
        return "RectifiedGridCoverage"
    
    def getType(self):
        """
        Returns ``eo.rect_dataset``
        """
        
        return "eo.rect_dataset"
    
    def getSize(self):
        """
        Returns the pixel size of the dataset as 2-tuple of integers
        ``(size_x, size_y)``.
        """
        
        return (self.__model.extent.size_x, self.__model.extent.size_y)

    #-------------------------------------------------------------------
    #  EOCoverageInterface implementations
    #-------------------------------------------------------------------
    
    def getEOCoverageSubtype(self):
        """
        Returns ``RectifiedDataset``.
        """
        
        return "RectifiedDataset"
    
    def getContainers(self):
        """
        This method returns a list of :class:`DatasetSeriesWrapper` and
        :class:`RectifiedStitchedMosaicWrapper` objects containing this
        Rectified Dataset, or an empty list.
        """
        cov_factory = System.getRegistry().bind(
            "resources.coverages.wrappers.EOCoverageFactory"
        )
        
        dss_factory = System.getRegistry().bind(
            "resources.coverages.wrappers.DatasetSeriesFactory"
        )
        
        self_expr = System.getRegistry().getFromFactory(
            "resources.coverages.filters.CoverageExpressionFactory",
            {"op_name": "contains", "operands": (self.__model.pk,)}
        )
        
        wrappers = []
        wrappers.extend(cov_factory.find(
            impl_ids=["resources.coverages.wrappers.RectifiedStitchedMosaicWrapper"],
            filter_exprs=[self_expr]
        ))
        wrappers.extend(dss_factory.find(filter_exprs=[self_expr]))
        
        return wrappers
    
    def getContainerCount(self):
        """
        This method returns the number of Dataset Series and 
        Rectified Stitched Mosaics containing this Rectified Dataset.
        """
        return self.__model.dataset_series_set.count() + \
               self.__model.rect_stitched_mosaics.count()
        
    def contains(self, res_id):
        """
        Always returns ``False``. A Dataset does not contain other
        Datasets.
        """
        return False
    
    def containedIn(self, res_id):
        """
        Returns ``True`` if this Rectified Dataset is contained in the
        Rectified Stitched Mosaic or Dataset Series with the resource
        primary key ``res_id``, ``False`` otherwise.
        """
        return self.__model.dataset_series_set.filter(pk=res_id).count() > 0 or \
               self.__model.rect_stitched_mosaics.filter(pk=res_id).count() > 0
    
RectifiedDatasetWrapperImplementation = \
RectifiedDatasetInterface.implement(RectifiedDatasetWrapper)

class ReferenceableDatasetWrapper(EODatasetWrapper, ReferenceableGridWrapper):
    """
    This is the wrapper for Referenceable Datasets. It inherits from
    :class:`EODatasetWrapper` and :class:`ReferenceableGridWrapper`.
    
    The following attributes are recognized:
    
    * ``eo_id``: the EO ID of the dataset; value must be a string
    * ``filename``: the path to the dataset; value must be a string
    * ``metadata_filename``: the path to the accompanying metadata
      file; value must be a string
    * ``size_x``: the width of the coverage in pixels; value must be
      an integer
    * ``size_y``: the height of the coverage in pixels; value must be
      an integer
    * ``visible``: the ``visible`` attribute of the dataset; value must
      be boolean
    * ``automatic``: the ``automatic`` attribute of the dataset; value
      must be boolean
    
    .. note:: The design of Referenceable Datasets is still TBD.
    """
    
    REGISTRY_CONF = {
        "name": "Referenceable Dataset Wrapper",
        "impl_id": "resources.coverages.wrappers.ReferenceableDatasetWrapper",
        "model_class": ReferenceableDatasetRecord,
        "id_field": "coverage_id",
        "factory_ids": ("resources.coverages.wrappers.EOCoverageFactory",)
    }
    
    FIELDS = {
        "eo_id": "eo_id",
        "filename": "file__path",
        "metadata_filename": "file__metadata_path",
        "metadata_format": "file__metadata_format",
        "size_x": "size_x",
        "size_y": "size_y",
        "visible": "visible",
        "automatic": "automatic"
    }
    
    #-------------------------------------------------------------------
    # ResourceInterface implementations
    #-------------------------------------------------------------------
    
    # NOTE: partially implemented in ResourceWrapper
    
    @property
    def __model(self):
        return self._ResourceWrapper__model
    
    #===========================================================================
    # TODO: update and extend this according to new data model
    
    def _createModel(self, params):
        pass # TODO
    
    def _updateModel(self, params):
        pass # TODO
    
    def _getAttrValue(self, attr_name):
        if attr_name == "eo_id":
            return self.__model.eo_id
        elif attr_name == "filename":
            return self.__model.file.path
        elif attr_name == "metadata_filename":
            return self.__model.file.metadata_path
        elif attr_name == "metadata_format":
            return self.__model.file.metadata_format
        elif attr_name == "size_x":
            return self.__model.size_x
        elif attr_name == "size_y":
            return self.__model.size_y
        elif attr_name == "visible":
            return self.__model.visible
        elif attr_name == "automatic":
            return self.__model.automatic
    
    def _setAttrValue(self, attr_name, value):
        if attr_name == "eo_id":
            self.__model.eo_id = value
        elif attr_name == "filename":
            self.__model.file.path = value
        elif attr_name == "metadata_filename":
            self.__model.file.metadata_path = value
        elif attr_name == "metadata_format":
            self.__model.file.metadata_format = value
        elif attr_name == "size_x":
            self.__model.size_x = value
        elif attr_name == "size_y":
            self.__model.size_y = value
        elif attr_name == "visible":
            self.__model.visible = value
        elif attr_name == "automatic":
            self.__model.automatic = value
    
    # END TODO
    #===========================================================================
    
    #-------------------------------------------------------------------
    # CoverageInterface implementations
    #-------------------------------------------------------------------
    
    # NOTE: partially implemented in CoverageWrapper
    
    def getCoverageSubtype(self):
        """
        Returns ``ReferenceableGridCoverage``.
        """
        
        return "ReferenceableGridCoverage"
    
    def getType(self):
        """
        Returns ``eo.ref_dataset``
        """
        
        return "eo.ref_dataset"
    
    def getSize(self):
        """
        Returns the pixel size of the dataset as 2-tuple of integers
        ``(size_x, size_y)``.
        """
        
        return (self.__model.size_x, self.__model.size_y)

    #-------------------------------------------------------------------
    #  EOCoverageInterface implementations
    #-------------------------------------------------------------------
    
    def getEOCoverageSubtype(self):
        """
        Returns ``ReferenceableDataset``.
        """
        
        return "ReferenceableDataset"

    def getContainers(self):
        """
        This method returns a list of :class:`DatasetSeriesWrapper`
        objects containing this Referenceable Dataset, or an empty list.
        """
        dss_factory = System.getRegistry().bind(
            "resources.coverages.wrappers.DatasetSeriesFactory"
        )
        
        self_expr = System.getRegistry().getFromFactory(
            "resources.coverages.filters.CoverageExpressionFactory",
            {"op_name": "contains", "operands": (self.__model.pk,)}
        )
        
        return dss_factory.find(filter_exprs=[self_expr])
    
    def getContainerCount(self):
        """
        This method returns the number of Dataset Series containing 
        this Referenceable Dataset.
        """
        return self.__model.dataset_series_set.count()
        
    def contains(self, res_id):
        """
        Always returns ``False``. A Dataset cannot contain other
        Datasets.
        """
        return False
    
    def containedIn(self, res_id):
        """
        This method returns ``True`` if this Referenceable Dataset is
        contained in the Dataset Series with the given resource
        primary key ``res_id``, ``False`` otherwise.
        """
        return self.__model.dataset_series_set.filter(pk=res_id).count() > 0

ReferenceableDatasetWrapperImplementation = \
ReferenceableDatasetInterface.implement(ReferenceableDatasetWrapper)

class RectifiedStitchedMosaicWrapper(TiledDataWrapper, RectifiedGridWrapper, EOCoverageWrapper):
    """
    This is the wrapper for Rectified Stitched Mosaics. It inherits
    from :class:`EOCoverageWrapper` and :class:`RectifiedGridWrapper`.
    It implements :class:`~.RectifiedStitchedMosaicInterface`.
    
    The following attributes are recognized:
    
    * ``eo_id``: the EO ID of the mosaic; value must be a string
    * ``srid``: the SRID of the mosaic's CRS; value must be an integer
    * ``size_x``: the width of the coverage in pixels; value must be
      an integer
    * ``size_y``: the height of the coverage in pixels; value must be
      an integer
    * ``minx``: the left hand bound of the mosaic's extent; value must
      be numeric
    * ``miny``: the lower bound of the mosaic's extent; value must be
      numeric
    * ``maxx``: the right hand bound of the mosaic's extent; value must
      be numeric
    * ``maxy``: the upper bound of the mosaic's extent; value must be
      numeric
    """

    REGISTRY_CONF = {
        "name": "Rectified Stitched Mosaic Wrapper",
        "impl_id": "resources.coverages.wrappers.RectifiedStitchedMosaicWrapper",
        "model_class": RectifiedStitchedMosaicRecord,
        "id_field": "coverage_id",
        "factory_ids": ("resources.coverages.wrappers.EOCoverageFactory",)
    }
    
    FIELDS = {
        "eo_id": "eo_id",
        "srid": "extent__srid",
        "size_x": "extent__size_x",
        "size_y": "extent__size_y",
        "minx": "extent__minx",
        "miny": "extent__miny",
        "maxx": "extent__maxx",
        "maxy": "extent__maxy",
    }

    #-------------------------------------------------------------------
    # ResourceInterface implementations
    #-------------------------------------------------------------------
    
    def __get_model(self):
        return self._ResourceWrapper__model
    
    def __set_model(self, model):
        self._ResourceWrapper__model = model
        
    __model = property(__get_model, __set_model)
    
    # NOTE: partially implemented in ResourceWrapper
    
    def _get_create_dict(self, params):
        create_dict = super(RectifiedStitchedMosaicWrapper, self)._get_create_dict(params)
        
        if "tile_index" not in params:
            raise InternalError(
                "Missing mandatory 'tile_index' parameter for RectifiedStitchedMosaic creation."
            )
        
        create_dict["tile_index"] = params["tile_index"].getRecord()
        
        
        
        return create_dict
    
    def _create_model(self, create_dict):
        self.__model = RectifiedStitchedMosaicRecord.objects.create(
            **create_dict
        )
        
    def _post_create(self, params):
        container = params.get("container")
        if container is not None:
            container.addCoverage(
                res_type=self.getType(),
                res_id=self.__model.pk
            )
        
        containers = params.get("containers", [])
        for container in containers:
            container.addCoverage(
                res_type=self.getType(),
                res_id=self.__model.pk
            )
            
        if "data_sources" in params:
            for data_source in params["data_sources"]:
                self.__model.data_sources.add(data_source.getRecord())
    
    def _updateModel(self, params):
        pass # TODO
    
    def _getAttrValue(self, attr_name):
        if attr_name == "eo_id":
            return self.__model.eo_id
        elif attr_name == "srid":
            return self.__model.extent.srid
        elif attr_name == "size_x":
            return self.__model.extent.size_x
        elif attr_name == "size_y":
            return self.__model.extent.size_y
        elif attr_name == "minx":
            return self.__model.extent.minx
        elif attr_name == "miny":
            return self.__model.extent.miny
        elif attr_name == "maxx":
            return self.__model.extent.maxx
        elif attr_name == "maxy":
            return self.__model.extent.maxy
    
    def _setAttrValue(self, attr_name, value):
        if attr_name == "eo_id":
            self.__model.eo_id = value
        elif attr_name == "srid":
            self.__model.extent.srid = value
        elif attr_name == "size_x":
            self.__model.extent.size_x = value
        elif attr_name == "size_y":
            self.__model.extent.size_y = value
        elif attr_name == "minx":
            self.__model.extent.minx = value
        elif attr_name == "miny":
            self.__model.extent.miny = value
        elif attr_name == "maxx":
            self.__model.extent.maxx = value
        elif attr_name == "maxy":
            self.__model.extent.maxy = value
    
    #-------------------------------------------------------------------
    # CoverageInterface implementations
    #-------------------------------------------------------------------
    
    # NOTE: partially implemented in CoverageWrapper
    
    def getCoverageSubtype(self):
        """
        Returns ``RectifiedGridCoverage``.
        """
        
        return "RectifiedGridCoverage"
    
    def getType(self):
        """
        Returns ``eo.rect_stitched_mosaic``
        """
        
        return "eo.rect_stitched_mosaic"
    
    def getSize(self):
        """
        Returns the pixel size of the mosaic as 2-tuple of integers
        ``(size_x, size_y)``.
        """
        
        return (self.__model.extent.size_x, self.__model.extent.size_y)

    #-------------------------------------------------------------------
    #  EOCoverageInterface implementations
    #-------------------------------------------------------------------
    
    def getEOCoverageSubtype(self):
        """
        Returns ``RectifiedStitchedMosaic``.
        """
        
        return "RectifiedStitchedMosaic"
    
    def getDatasets(self, filter_exprs=None):
        """
        Returns a list of :class:`RectifiedDatasetWrapper` objects
        contained in the stitched mosaic wrapped by the implementation.
        It accepts an optional ``filter_exprs`` parameter which is
        expected to be a list of filter expressions
        (see module :mod:`eoxserver.resources.coverages.filters`) or
        ``None``. Only the datasets matching the filters will be
        returned; in case no matching coverages are found an empty list
        will be returned.
        """

        _filter_exprs = []
        
        if filter_exprs is not None:
            _filter_exprs.extend(filter_exprs)
        
        self_expr = System.getRegistry().getFromFactory(
            "resources.coverages.filters.CoverageExpressionFactory",
            {"op_name": "contained_in", "operands": (self.__model.pk,)}
        )
        _filter_exprs.append(self_expr)
                
        factory = System.getRegistry().bind(
            "resources.coverages.wrappers.EOCoverageFactory"
        )
        
        return factory.find(
            impl_ids=["resources.coverages.wrappers.RectifiedDatasetWrapper"],
            filter_exprs=_filter_exprs
        )
        
    def getContainers(self):
        """
        This method returns a list of :class:`DatasetSeriesWrapper`
        objects containing this Stitched Mosaic or an empty list.
        """
        
        dss_factory = System.getRegistry().bind(
            "resources.coverages.wrappers.DatasetSeriesFactory"
        )
        
        self_expr = System.getRegistry().getFromFactory(
            "resources.coverages.filters.CoverageExpressionFactory",
            {"op_name": "contains", "operands": (self.__model.pk,)}
        )
        
        return dss_factory.find(filter_exprs=[self_expr])
    
    def getContainerCount(self):
        """
        This method returns the number of Dataset Series containing
        this Stitched Mosaic.
        """
        return self.__model.dataset_series_set.count()
        
    def contains(self, res_id):
        """
        This method returns ``True`` if the a Rectified Dataset with
        resource primary key ``res_id`` is contained within this
        Stitched Mosaic, ``False`` otherwise.
        """
        return self.__model.rect_datasets.filter(pk=res_id).count() > 0
    
    def containedIn(self, res_id):
        """
        This method returns ``True`` if this Stitched Mosaic is 
        contained in the Dataset Series with resource primary key
        ``res_id``, ``False`` otherwise.
        """
        return self.__model.dataset_series_set.filter(pk=res_id).count() > 0
    
    #-------------------------------------------------------------------
    # RectifiedStitchedMosaicInterface methods
    #-------------------------------------------------------------------
    
    def getShapeFilePath(self):
        """
        Returns the path to the shape file.
        """
        return os.path.join(self.__model.storage_dir, "tindex.shp")
    
    def addCoverage(self, res_type, res_id):
        """
        Adds a Rectified Dataset with primary key ``res_id`` to the
        Rectified Stitched Mosaic. An :exc:`InternalError` is raised if
        the ``res_type`` is not equal to ``eo.rect_dataset``.
        """
        if res_type != "eo.rect_dataset":
            raise InternalError(
                "Cannot add coverages of type '%s' to Rectified Stitched Mosaics" %\
                res_type
            )
        else:
            self.__model.rect_datasets.add(res_id)
    
    def removeCoverage(self, res_type, res_id):
        """
        Removes a Rectified Dataset with primary key ``res_id`` from the
        Rectified Stitched Mosaic. An :exc:`InternalError` is raised if
        the ``res_type`` is not equal to ``eo.rect_dataset``.
        """
        if res_type != "eo.rect_dataset":
            raise InternalError(
                "Cannot remove coverages of type '%s' from Rectified Stitched Mosaics" %\
                res_type
            )
        else:
            self.__model.rect_datasets.remove(res_id)
    
    def getDataDirs(self):
        """
        This method returns a list of directories which hold the 
        stitched mosaic data.
        """
        
        return list(
            self.__model.data_dirs.values_list('dir', flat=True)
        )

    def getDataSources(self):
        data_source_factory = System.getRegistry().bind(
            "resources.coverages.data.DataSourceFactory"
        )
        return [data_source_factory.get(record=record) 
                for record in self.__model.data_sources.all()]
        
    
    def getImagePattern(self):
        """
        Returns the filename pattern for image files to be included 
        into the stitched mosaic. The pattern is expressed in the format
        accepted by :func:`fnmatch.fnmatch`.
        """
        
        return self.__model.image_pattern
        
RectifiedStitchedMosaicWrapperImplementation = \
RectifiedStitchedMosaicInterface.implement(RectifiedStitchedMosaicWrapper)

class DatasetSeriesWrapper(EOMetadataWrapper, ResourceWrapper):
    """
    This is the wrapper for Dataset Series. It inherits from
    :class:`EOMetadataWrapper`. It implements
    :class:`DatasetSeriesInterface`.
    """
    
    REGISTRY_CONF = {
        "name": "Dataset Series Wrapper",
        "impl_id": "resources.coverages.wrappers.DatasetSeriesWrapper",
        "model_class": DatasetSeriesRecord,
        "id_field": "eo_id",
        "factory_ids": ("resources.coverages.wrappers.DatasetSeriesFactory", )
    }
    
    FIELDS = {}
    
    #-------------------------------------------------------------------
    # ResourceInterface methods
    #-------------------------------------------------------------------
    
    # NOTE: partially implemented by ResourceWrapper
    
    def __get_model(self):
        return self._ResourceWrapper__model
    
    def __set_model(self, model):
        self._ResourceWrapper__model = model
        
    __model = property(__get_model, __set_model)
        
    def _get_create_dict(self, params):
        create_dict = super(DatasetSeriesWrapper, self)._get_create_dict(params)
        
        layer_metadata = params.get("layer_metadata")
        
        if layer_metadata:
            create_dict["layer_metadata"] = []
            
            for key, value in layer_metadata.items():
                create_dict["layer_metadata"].append(
                    LayerMetadataRecord.objects.get_or_create(
                        key=key, value=value
                    )[0]
                )
        
        return create_dict
    
    def _create_model(self, create_dict):
        self.__model = DatasetSeriesRecord.objects.create(**create_dict)
    
    def _post_create(self, params):
        try:
            for data_source in params["data_sources"]:
                self.__model.data_sources.add(data_source.getRecord())
        except KeyError:
            pass
    
    # use default _post_create() which does nothing
        
    def _updateModel(self, params):
        pass # TODO
        
    def _getAttrValue(self, attr_name):
        pass # TODO
        
    def _setAttrValue(self, attr_name, value):
        pass # TODO
    
    #-------------------------------------------------------------------
    # DatasetSeriesInterface methods
    #-------------------------------------------------------------------
    
    def getType(self):
        """
        Returns ``"eo.dataset_series"``.
        """
        
        return "eo.dataset_series"
    
    def getEOCoverages(self, filter_exprs=None):
        """
        This method returns a list of EOCoverage wrappers (for datasets
        and stitched mosaics) associated with the dataset series wrapped
        by the implementation. It accepts an optional ``filter_exprs``
        parameter which is expected to be a list of filter expressions
        (see module :mod:`eoxserver.resources.coverages.filters`) or
        ``None``. Only the EOCoverages matching the filters will be
        returned; in case no matching coverages are found an empty list
        will be returned.
        """
        
        _filter_exprs = []

        if filter_exprs is not None:
            _filter_exprs.extend(filter_exprs)
        
        self_expr = System.getRegistry().getFromFactory(
            "resources.coverages.filters.CoverageExpressionFactory",
            {"op_name": "contained_in", "operands": (self.__model.pk,)}
        )
        _filter_exprs.append(self_expr)
        
        factory = System.getRegistry().bind(
            "resources.coverages.wrappers.EOCoverageFactory"
        )
        
        return factory.find(filter_exprs=_filter_exprs)
        
    def contains(self, res_id):
        """
        This method returns ``True`` if the Dataset Series contains
        the EO Coverage with resource primary key ``res_id``, ``False``
        otherwise.
        """
        return self.__model.rect_datasets.filter(pk=res_id).count() > 0 or \
               self.__model.ref_datasets.filter(pk=res_id).count() > 0 or \
               self.__model.rect_stitched_mosaics.filter(pk=res_id).count() > 0
    
    def addCoverage(self, res_type, res_id):
        """
        Adds the EO coverage of type ``res_type`` with primary key
        ``res_id`` to the dataset series. An :exc:`InternalError` is
        raised if the type cannot be handled by Dataset Series.
        Supported types are:
        
        * ``eo.rect_dataset``
        * ``eo.ref_dataset``
        * ``eo.rect_stitched_mosaic``
        """
        if res_type == "eo.rect_dataset":
            self.__model.rect_datasets.add(res_id)
        elif res_type == "eo.ref_dataset":
            self.__model.ref_datasets.add(res_id)
        elif res_type == "eo.rect_stitched_mosaic":
            self.__model.rect_stitched_mosaics.add(res_id)
        else:
            raise InternalError(
                "Cannot add coverages of type '%s' to Dataset Series" %\
                res_type
            )

    def removeCoverage(self, res_type, res_id):
        """
        Removes the EO coverage of type ``res_type`` with primary key
        ``res_id`` from the dataset series. An :exc:`InternalError` is
        raised if the type cannot be handled by Dataset Series.
        Supported types are:
        
        * ``eo.rect_dataset``
        * ``eo.ref_dataset``
        * ``eo.rect_stitched_mosaic``
        """
        if res_type == "eo.rect_dataset":
            self.__model.rect_datasets.remove(res_id)
        elif res_type == "eo.ref_dataset":
            self.__model.ref_datasets.remove(res_id)
        elif res_type == "eo.rect_stitched_mosaic":
            self.__model.rect_stitched_mosaics.remove(res_id)
        else:
            raise InternalError(
                "Cannot add coverages of type '%s' to Dataset Series" %\
                res_type
            )
    
    def getDataDirs(self):
        """
        This method returns a list of directories which hold the 
        dataset series data.
        """
        
        return list(
            self.__model.data_dirs.values_list('dir', flat=True)
        )
    
    def getDataSources(self):
        data_source_factory = System.getRegistry().bind(
            "resources.coverages.data.DataSourceFactory"
        )
        return [data_source_factory.get(record=record) 
                for record in self.__model.data_sources.all()]
    
    def getImagePattern(self):
        """
        Returns the filename pattern for image files to be included 
        into the stitched mosaic. The pattern is expressed in the format
        accepted by :func:`fnmatch.fnmatch`.
        """
        
        return self.__model.image_pattern

DatasetSeriesWrapperImplementation = \
DatasetSeriesInterface.implement(DatasetSeriesWrapper)

#-----------------------------------------------------------------------
# Factories
#-----------------------------------------------------------------------

class EOCoverageFactory(ResourceFactory):
    REGISTRY_CONF = {
        "name": "EO Coverage Factory",
        "impl_id": "resources.coverages.wrappers.EOCoverageFactory"
    }

EOCoverageFactoryImplementation = \
ResourceFactoryInterface.implement(EOCoverageFactory)

class DatasetSeriesFactory(ResourceFactory):
    REGISTRY_CONF = {
        "name": "Dataset Series Factory",
        "impl_id": "resources.coverages.wrappers.DatasetSeriesFactory"
    }

DatasetSeriesFactoryImplementation = \
ResourceFactoryInterface.implement(DatasetSeriesFactory)
