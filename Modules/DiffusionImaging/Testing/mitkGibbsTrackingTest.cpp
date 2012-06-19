/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#include "mitkTestingMacros.h"

#include <mitkDiffusionImagingObjectFactory.h>
#include <mitkFiberBundleX.h>
#include <mitkFiberBundleXReader.h>
#include <mitkQBallImage.h>
#include <mitkNrrdQBallImageReader.h>
#include <mitkImage.h>
#include <mitkItkImageFileReader.h>
#include <itkGibbsTrackingFilter.h>
#include <itkOrientationDistributionFunction.h>
#include <mitkBaseDataIOFactory.h>
#include <mitkBaseData.h>
#include <itksys/SystemTools.hxx>
#include <mitkTestingConfig.h>
#include <math.h>
#include <tinyxml.h>
#include <mitkImageCast.h>

using namespace mitk;

/**Documentation
 *  Test for gibbs tracking filter
 */
int mitkGibbsTrackingTest(int argc, char* argv[])
{
  MITK_TEST_BEGIN("mitkGibbsTrackingTest");

  MITK_TEST_CONDITION_REQUIRED(argc>5,"check for input data")

  QBallImage::Pointer mitkQballImage;
  Image::Pointer mitkMaskImage;
  mitk::FiberBundleX::Pointer fib1;

  try{
    RegisterDiffusionImagingObjectFactory();

    // test if fib1 can be read
    const std::string s1="", s2="";
    std::vector<mitk::BaseData::Pointer> infile = mitk::BaseDataIO::LoadBaseDataFromFile( argv[1], s1, s2, false );
    mitkQballImage = dynamic_cast<mitk::QBallImage*>(infile.at(0).GetPointer());
    MITK_TEST_CONDITION_REQUIRED(mitkQballImage.IsNotNull(),"check qball image")

    infile = mitk::BaseDataIO::LoadBaseDataFromFile( argv[2], s1, s2, false );
    mitkMaskImage = dynamic_cast<mitk::Image*>(infile.at(0).GetPointer());
    MITK_TEST_CONDITION_REQUIRED(mitkMaskImage.IsNotNull(),"check qball image")

    infile = mitk::BaseDataIO::LoadBaseDataFromFile( argv[5], s1, s2, false );
    fib1 = dynamic_cast<mitk::FiberBundleX*>(infile.at(0).GetPointer());
    MITK_TEST_CONDITION_REQUIRED(fib1.IsNotNull(),"check fiber bundle")

    typedef itk::Vector<float, QBALL_ODFSIZE> OdfVectorType;
    typedef itk::Image<OdfVectorType,3> OdfVectorImgType;
    typedef itk::Image<float,3> MaskImgType;
    typedef itk::GibbsTrackingFilter<OdfVectorImgType> GibbsTrackingFilterType;

    OdfVectorImgType::Pointer itk_qbi = OdfVectorImgType::New();
    mitk::CastToItkImage<OdfVectorImgType>(mitkQballImage, itk_qbi);

    MaskImgType::Pointer itk_mask = MaskImgType::New();
    mitk::CastToItkImage<MaskImgType>(mitkMaskImage, itk_mask);

    GibbsTrackingFilterType::Pointer gibbsTracker = GibbsTrackingFilterType::New();
    gibbsTracker->SetQBallImage(itk_qbi.GetPointer());
    gibbsTracker->SetMaskImage(itk_mask);
    gibbsTracker->SetDuplicateImage(false);
    gibbsTracker->SetRandomSeed(1);
    gibbsTracker->SetParameterFile(std::string(argv[3]));
    gibbsTracker->SetLutPath(std::string(argv[4]));
    gibbsTracker->Update();

    mitk::FiberBundleX::Pointer fib2 = mitk::FiberBundleX::New(gibbsTracker->GetFiberBundle());
    MITK_TEST_CONDITION_REQUIRED(fib1->Equals(fib2), "check if gibbs tracking has changed");
  }
  catch(...)
  {
    return EXIT_FAILURE;
  }

  // always end with this!
  MITK_TEST_END();
}