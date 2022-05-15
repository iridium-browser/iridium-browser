CPPSRC= \
src/ggCoverageSolidTexture.cc \
src/ggPathDielectricMaterial.cc \
src/ggBox2.cc \
src/ggBox3.cc \
src/ggRasterSurfaceTexture.cc \
src/ggJitterSample1.cc \
src/ggNRooksSample2.cc \
src/ggJitterSample2.cc \
src/ggRGBFPixel.cc \
src/ggRotatingPinholeCamera.cc \
src/ggRandomSample2.cc \
src/ggRandomSample3.cc \
src/ggSolidNoise2.cc \
src/ggSolidNoise3.cc \
src/ggSolidTexture.cc \
src/ggOakSolidTexture.cc \
src/ggSpecularMaterial.cc \
src/ggNA.cc \
src/ggMaterial.cc \
src/ggDiffuseMaterial.cc \
src/ggDielectricMaterial.cc \
src/ggConductorMaterial.cc \
src/ggEdgeDiscrepancy.cc \
src/ggPolishedMaterial.cc \
src/ggFrameInterpolation.cc \
src/ggQuaternion.cc \
src/ggSpline.cc \
src/ggONB2.cc \
src/ggONB3.cc \
src/ggFrame2.cc \
src/ggFrame3.cc \
src/ggHMatrix3.cc \
src/ggSphere.cc \
src/ggRGBE.cc \
src/ggGamma.cc \
src/ggBRDF.cc \
src/ggDiffuseBRDF.cc \
src/ggPhongBRDF.cc \
src/ggPolishedBRDF.cc \
src/ggCamera.cc \
src/ggPinholeCamera.cc \
src/ggThinLensCamera.cc \
src/ggErr.cc \
src/ggPixelFilter.cc \
src/ggTrianglePixelFilter.cc \
src/ggBoxPixelFilter.cc \
src/ggFormat.cc \
src/ggGeometry.cc \
src/ggHAffineMatrix3.cc \
src/ggHShearMatrix3.cc \
src/ggHBoxMatrix3.cc \
src/ggHPerspectiveMatrix3.cc \
src/ggHTranslationMatrix3.cc \
src/ggHRigidBodyMatrix3.cc \
src/ggHRotationMatrix3.cc \
src/ggHScaleMatrix3.cc \
src/ggHPoint3.cc \
src/ggPoint2.cc \
src/ggPoint3.cc \
src/ggOptics.cc \
src/ggPlane.cc \
src/ggPolygon.cc \
src/ggGrayPixel_x.cc \
src/ggRGBPixel_x.cc \
src/ggRay2.cc \
src/ggRay3.cc \
src/ggBoardFloorSolidTexture.cc \
src/ggVector2.cc \
src/ggVector3.cc \
src/ggString.cc \
src/ggFineSpectrum.cc \
src/ggSpectrum.cc \
src/eon.cc \
src/eonImageCalculator.cc \
src/mrObjectRecord.cc \
src/mrCoarsePixelRenderer.cc \
src/mrIndirectPixelRenderer.cc \
src/mrRushmeierPixelRenderer.cc \
src/mrBruteForcePixelRenderer.cc \
src/mrKajiyaPixelRenderer.cc \
src/mrCookPixelRenderer.cc \
src/mrGrid.cc \
src/mrScene.cc \
src/mrCamera.cc \
src/mrTriangle.cc \
src/mrFastTriangle.cc \
src/mrPolygon.cc \
src/mrXYDisk.cc \
src/mrXZDisk.cc \
src/mrYZDisk.cc \
src/mrPhongAreaXYRectangleLuminaire.cc \
src/mrPhongAreaXZRectangleLuminaire.cc \
src/mrPhongAreaYZRectangleLuminaire.cc \
src/mrEmitter.cc \
src/mrImposter.cc \
src/mrLinkedObjects.cc \
src/mrBox.cc \
src/mrXCylinder.cc \
src/mrYCylinder.cc \
src/mrZCylinder.cc \
src/mrDiffuseAreaZCylinderLuminaire.cc \
src/mrDiffuseCosineZCylinderLuminaire.cc \
src/mrDiffuseVisibleAreaZCylinderLuminaire.cc \
src/mrXEllipticalCylinder.cc \
src/mrYEllipticalCylinder.cc \
src/mrZEllipticalCylinder.cc \
src/mrSpotAreaXYDiskLuminaire.cc \
src/mrDiffuseAreaXYRectangleLuminaire.cc \
src/mrDiffuseAreaXZRectangleLuminaire.cc \
src/mrDiffuseAreaYZRectangleLuminaire.cc \
src/mrSurfaceList.cc \
src/mrDiffuseAreaTriangleLuminaire.cc \
src/mrDiffuseAreaSphereLuminaire.cc \
src/mrDiffuseCosineSphereLuminaire.cc \
src/mrDiffuseSolidAngleSphereLuminaire.cc \
src/mrShellLuminaire.cc \
src/mrInstance.cc \
src/mrMaterial.cc \
src/mrPhongAreaTriangleLuminaire.cc \
src/mrSolidTexture.cc \
src/mrSphere.cc \
src/mrSurface.cc \
src/mrSurfaceTexture.cc \
src/mrXYRectangle.cc \
src/mrXZRectangle.cc \
src/mrYZRectangle.cc \
src/myrand.cc


IT=eon
CFLAGS_EXTRA = -DNDEBUG -Isrc  -DUSE_STRERROR -DHAS_ERRLIST
EMCCFLAGS_EXTRA_TRAIN = \
--preload-file chair.camera \
--preload-file chair.control.cook \
--preload-file chair.control.kajiya \
--preload-file chair.control.rushmeier \
--preload-file chair.surfaces \
--preload-file eon.dat \
--preload-file materials \
--preload-file spectra.dat
EMCCFLAGS_EXTRA_REF = $(EMCCFLAGS_EXTRA_TRAIN)

EXTRA_CLEAN= pixels* chair* materials spectra.dat
include ../Makefile.common
