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

#include "vtkNeverTranslucentTexture.h"

#include "vtkVersion.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkImageData.h"
#include "vtkScalarsToColors.h"
#include "vtkTextureObject.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkImageCast.h"
#include "vtkOpenGLError.h"

// Replacement texture code -------------------------------------------------------------

vtkStandardNewMacro(vtkNeverTranslucentTexture);

vtkNeverTranslucentTexture::vtkNeverTranslucentTexture()
:vtkOpenGLTexture()
{
}


int vtkNeverTranslucentTexture::IsTranslucent()
{
  return 0; // THE speedup
}


void vtkNeverTranslucentTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Translucent: NEVER\n";

}

// Factory code -------------------------------------------------------------

VTK_CREATE_CREATE_FUNCTION(vtkNeverTranslucentTexture);

vtkNeverTranslucentTextureFactory::vtkNeverTranslucentTextureFactory()
{
  this->RegisterOverride("vtkTexture",
                         "vtkNeverTranslucentTextureFactory",
                         "less translucent texture",
                         1,
                         vtkObjectFactoryCreatevtkNeverTranslucentTexture);
}

vtkNeverTranslucentTextureFactory* vtkNeverTranslucentTextureFactory::New()
{
  return new vtkNeverTranslucentTextureFactory;
}

const char* vtkNeverTranslucentTextureFactory::GetVTKSourceVersion()
{
  return VTK_SOURCE_VERSION;
}

const char* vtkNeverTranslucentTextureFactory::GetDescription()
{
  return "Factory for a quickly decided, never translucent, texture";
}

// ----------------------------------------------------------------------------
// Implement base class method.
void vtkNeverTranslucentTexture::Load(vtkRenderer *ren)
{
  if (!this->ExternalTextureObject)
    {
    vtkImageData *input = this->GetInput();

    // Need to reload the texture.
    // There used to be a check on the render window's mtime, but
    // this is too broad of a check (e.g. it would cause all textures
    // to load when only the desired update rate changed).
    // If a better check is required, check something more specific,
    // like the graphics context.
    vtkOpenGLRenderWindow* renWin = static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow());

    // has something changed so that we need to rebuild the texture?
    if (this->GetMTime() > this->LoadTime.GetMTime() ||
        input->GetMTime() > this->LoadTime.GetMTime() ||
        (this->GetLookupTable() && this->GetLookupTable()->GetMTime () >
         this->LoadTime.GetMTime()) ||
         renWin != this->RenderWindow.GetPointer() ||
         renWin->GetContextCreationTime() > this->LoadTime)
      {
      int size[3];
      unsigned char *dataPtr;
      unsigned char *resultData = 0;
      int xsize, ysize;
      int channelByteSize;

      this->RenderWindow = renWin;
      if (this->TextureObject == 0)
        {
        this->TextureObject = vtkTextureObject::New();
        }
      this->TextureObject->ResetFormatAndType();
      this->TextureObject->SetContext(renWin);

      // Get the scalars the user choose to color with.
      vtkDataArray* scalars = this->GetInputArrayToProcess(0, input);

      // make sure scalars are non null
      if (!scalars)
        {
        vtkErrorMacro(<< "No scalar values found for texture input!");
        return;
        }

      // get some info
      input->GetDimensions(size);

      if (input->GetNumberOfCells() == scalars->GetNumberOfTuples())
        {
        // we are using cell scalars. Adjust image size for cells.
        for (int kk = 0; kk < 3; kk++)
          {
          if (size[kk]>1)
            {
            size[kk]--;
            }
          }
        }

      int bytesPerPixel = scalars->GetNumberOfComponents();

      // make sure using unsigned char data of color scalars type
      if (this->IsDepthTexture != 1 &&
        (this->MapColorScalarsThroughLookupTable ||
         (scalars->GetDataType() != VTK_UNSIGNED_CHAR && scalars->GetDataType() != VTK_FLOAT) ))
        {
        dataPtr = this->MapScalarsToColors (scalars);
        bytesPerPixel = 4;
        }
      else
        {

        if (dynamic_cast<vtkFloatArray *>(scalars))
        {
          std::cout << "float mode " << std::endl;
          dataPtr = reinterpret_cast<unsigned char *>(static_cast<vtkFloatArray *>(scalars)->GetPointer(0));
          channelByteSize = 4;
        }
        else
        {
          dataPtr = static_cast<vtkUnsignedCharArray *>(scalars)->GetPointer(0);
          channelByteSize = 1;
        }

        }

      // we only support 2d texture maps right now
      // so one of the three sizes must be 1, but it
      // could be any of them, so lets find it
      if (size[0] == 1)
        {
        xsize = size[1]; ysize = size[2];
        }
      else
        {
        xsize = size[0];
        if (size[1] == 1)
          {
          ysize = size[2];
          }
        else
          {
          ysize = size[1];
          if (size[2] != 1)
            {
            vtkErrorMacro(<< "3D texture maps currently are not supported!");
            return;
            }
          }
        }

      // -- decide whether the texture needs to be resampled --
      GLint maxDimGL;
      glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxDimGL);
      vtkOpenGLCheckErrorMacro("failed at glGetIntegerv");
      // if larger than permitted by the graphics library then must resample
      bool resampleNeeded = xsize > maxDimGL || ysize > maxDimGL;
      if(resampleNeeded)
        {
        vtkDebugMacro( "Texture too big for gl, maximum is " << maxDimGL);
        }

      if (resampleNeeded)
        {
        vtkDebugMacro(<< "Resampling texture to power of two for OpenGL");
        resultData = this->ResampleToPowerOfTwo(xsize, ysize, dataPtr,
                                                bytesPerPixel);
        }

      if (!resultData)
        {
        resultData = dataPtr;
        }

      // create the texture
      if (this->IsDepthTexture)
        {
        this->TextureObject->CreateDepthFromRaw(
          xsize, ysize, vtkTextureObject::Float32, scalars->GetDataType(), resultData);
        }
      else
        {
          if(channelByteSize==4)
          {
            this->TextureObject->Create2DFromRaw(
              xsize, ysize, bytesPerPixel, VTK_FLOAT, resultData);
          }
          else
          {
          this->TextureObject->Create2DFromRaw(
            xsize, ysize, bytesPerPixel, VTK_UNSIGNED_CHAR, resultData);
          }
        }

      // activate a free texture unit for this texture
      this->TextureObject->Activate();

      // update parameters
      if (this->Interpolate)
        {
        this->TextureObject->SetMinificationFilter(vtkTextureObject::Linear);
        this->TextureObject->SetMagnificationFilter(vtkTextureObject::Linear);
        }
      else
        {
        this->TextureObject->SetMinificationFilter(vtkTextureObject::Nearest);
        this->TextureObject->SetMagnificationFilter(vtkTextureObject::Nearest);
        }
      if (this->Repeat)
        {
        this->TextureObject->SetWrapS(vtkTextureObject::Repeat);
        this->TextureObject->SetWrapT(vtkTextureObject::Repeat);
        this->TextureObject->SetWrapR(vtkTextureObject::Repeat);
        }
      else
        {
        this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
        this->TextureObject->SetWrapT(vtkTextureObject::ClampToEdge);
        this->TextureObject->SetWrapR(vtkTextureObject::ClampToEdge);
        }

      // modify the load time to the current time
      this->LoadTime.Modified();

      // free memory
      if (resultData != dataPtr)
        {
        delete [] resultData;
        resultData = 0;
        }
      }
    }
  else
    {
    vtkOpenGLRenderWindow* renWin =
      static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow());

      // has something changed so that we need to rebuild the texture?
      if (this->GetMTime() > this->LoadTime.GetMTime() ||
         renWin != this->RenderWindow.GetPointer() ||
         renWin->GetContextCreationTime() > this->LoadTime)
        {
        this->RenderWindow = renWin;
        this->TextureObject->SetContext(renWin);
        }
    }

  // activate a free texture unit for this texture
  this->TextureObject->Activate();

  if (this->PremultipliedAlpha)
    {
    // make the blend function correct for textures premultiplied by alpha.
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

  vtkOpenGLCheckErrorMacro("failed after Load");
}
