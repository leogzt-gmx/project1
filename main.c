#include <stdio.h>
#include "gdal.h"
#include "gdal_alg.h"
#include "cpl_conv.h" /* for CPLMalloc() */
#include "ogr_srs_api.h"
#include "ogr_core.h"

void createContoursAndPoints(const char *);

int main()
{
    const char * rasterFile = 
    "/home/tiger/D/WORK/BGC/CCM_Geany/data/subset_square2.tif";

    createContoursAndPoints(rasterFile); 
    return 0;
}

void createContoursAndPoints(const char * rasterFile)
{
    GDALAllRegister();
    
    /*  ........... Open raster DEM ............. */
    GDALDatasetH rasterDataset = GDALOpen(rasterFile, GA_ReadOnly);    
    if( rasterDataset == NULL )
    {
        printf("Cannot open raster dataset\n");
        return;
    }
    
    GDALRasterBandH rasterBand = GDALGetRasterBand( rasterDataset, 1 );
    if( rasterBand == NULL )
    {
        printf("Band doesn't exist in dataset\n");
    }
    
    /*  ........... Get raster projection ............. */
    OGRSpatialReferenceH spatialReference = NULL;
    const char * rasterProjection = GDALGetProjectionRef(rasterDataset);
    spatialReference = OSRNewSpatialReference(rasterProjection);
    
    /*  ........... Get No Data Value ............. */  
    int bNoDataSet = 0;  
    double rasterNoDataValue = GDALGetRasterNoDataValue(rasterBand, &bNoDataSet);
    
    /*  ........... Create contours shapefile ............. */
    const char * pszDriverName = "ESRI Shapefile";
    
    GDALDriverH hDriver = GDALGetDriverByName( pszDriverName );
    if( hDriver == NULL )
    {
        printf( "%s driver not available.\n", pszDriverName );
    }
    
    GDALDatasetH contourDataset = GDALCreate(hDriver, 
        "/home/tiger/D/WORK/BGC/CCM_Geany/code/output_files/contours.shp",
         0, 0, 0, GDT_Unknown, NULL);
    if(contourDataset == NULL)
    {
        printf("Creation of contour file failed.\n");
    }
    
    OGRLayerH contourLayer = GDALDatasetCreateLayer(contourDataset, "contour", 
        spatialReference, wkbLineString, NULL );
    if( contourLayer == NULL )
    {
        printf( "Layer creation failed.\n" );
    }
    
    OGRFieldDefnH hFieldDefn = OGR_Fld_Create("ID", OFTInteger);
    //OGR_Fld_SetWidth( hFieldDefn, 32); check out this just in case there is untraceable bug with the shapefiles
    if( OGR_L_CreateField(contourLayer, hFieldDefn, TRUE ) != OGRERR_NONE )
    {
        printf( "Creating ID field failed.\n" );
        exit( 1 );
    }
    
    hFieldDefn = OGR_Fld_Create("ELEV", OFTInteger);
    if( OGR_L_CreateField(contourLayer, hFieldDefn, TRUE) != OGRERR_NONE)
    {
        printf( "Creating ELEV field failed.\n" );
        exit( 1 );
    }
    //OGR_Fld_Destroy(hFieldDefn);
    
    /*  ........... Get field index ............. */
    int indexIdField = OGR_FD_GetFieldIndex(OGR_L_GetLayerDefn(contourLayer), 
        "ID" );
    int indexElevField = OGR_FD_GetFieldIndex(OGR_L_GetLayerDefn(contourLayer), 
        "ELEV" );	
	
    /*  ........... Generate Contours ............. */
    CPLErr eErr = GDALContourGenerate(rasterBand, 10, 0, 0, NULL, TRUE, 
        rasterNoDataValue, contourLayer, indexIdField, indexElevField, NULL, 
        NULL);
	
	if (eErr != 0)
	{
		printf("Error: %d", eErr);
	}
	
	GDALClose(rasterDataset); // we domn't need anymore the raster dataset
	
    /*  ........... Create point shapefile ............. */    
    GDALDatasetH pointDataset = GDALCreate(hDriver, 
        "/home/tiger/D/WORK/BGC/CCM_Geany/code/output_files/points.shp",
         0, 0, 0, GDT_Unknown, NULL);
    if(pointDataset == NULL)
    {
        printf("Creation of point file failed.\n");
    }
    
    OGRLayerH pointLayer = GDALDatasetCreateLayer(pointDataset, "point", 
        spatialReference, wkbPoint, NULL);
    if(pointLayer == NULL)
    {
        printf("Layer creation failed.\n");
    }
    
    //hFieldDefn = OGR_Fld_Create("ID", OFTInteger);
    //if (OGR_L_CreateField(pointLayer, hFieldDefn, TRUE) != OGRERR_NONE )
    //{
        //printf( "Creating ID field failed.\n" );
        //exit( 1 );
    //}
    hFieldDefn = OGR_Fld_Create("ELEV", OFTInteger);
    if (OGR_L_CreateField(pointLayer, hFieldDefn, TRUE) != OGRERR_NONE )
    {
        printf( "Creating ELEV field failed.\n" );
        exit( 1 );
    }
    OGR_Fld_Destroy(hFieldDefn);
    
    /*  ........... Populate points layer ............. */
    OGRFeatureH contourFeature;
    OGRFeatureH pointFeature;
	OGR_L_ResetReading(contourLayer);
	OGR_L_ResetReading(pointLayer);
	
	const float distStep = 10.0;
	
	while((contourFeature = OGR_L_GetNextFeature(contourLayer)) != NULL)
	{
		OGRGeometryH contourGeometry = OGR_F_GetGeometryRef(contourFeature);
		double contourLength = OGR_G_Length(contourGeometry);
		double distAlongContour = 0.0;	
		
		int contourElev = OGR_F_GetFieldAsInteger(contourFeature, 1); // hardcoded again the fiesld of the shapefiles
		//int indexPoint = 0;
		
		while(distAlongContour <= contourLength) 
		{
			OGRGeometryH pointGeometry = OGR_G_Value(contourGeometry, distAlongContour);
			if(pointGeometry != NULL)
			{
				double xCoord = OGR_G_GetX(pointGeometry, 0);
				double yCoord = OGR_G_GetY(pointGeometry, 0);				
	
				pointFeature = OGR_F_Create(OGR_L_GetLayerDefn(pointLayer));
				OGR_G_SetPoint_2D(pointGeometry, 0, xCoord, yCoord);
				OGR_F_SetGeometry(pointFeature, pointGeometry);
				OGR_G_DestroyGeometry(pointGeometry);
				
				/*  ........... Fill ID and elev in point shapefile ............. */
				//OGR_F_SetFieldInteger(pointFeature, 0, indexPoint); 
				OGR_F_SetFieldInteger(pointFeature, 0, contourElev); 		
				
				
				if(OGR_L_CreateFeature(pointLayer, pointFeature) != OGRERR_NONE)
				{
				   printf("Failed to create point feature in shapefile.\n");
				   exit( 1 );
				}
				//OGR_F_Destroy(pointFeature);	// do not remove those yet, we are not sure if OGR_F_Destroy should be put here or outside the while loop
			}
			distAlongContour = distAlongContour + distStep;
			//indexPoint++;			
		}
		//OGR_F_Destroy(contourFeature);		
	}	
	OGR_F_Destroy(pointFeature);
	OGR_F_Destroy(contourFeature);
	
	GDALClose(contourDataset);
	GDALClose(pointDataset);
	OSRDestroySpatialReference(spatialReference); 
	GDALDestroyDriverManager();  
	OGRCleanupAll();
}
