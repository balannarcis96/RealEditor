#include "TextureProcessor.h"

#include <Tera/ALog.h>
#include <Tera/FStream.h>

#include <nvtt/nvtt.h>
#include <FreeImage.h>

#include <ppl.h>

// freeimage raii container
struct FreeImageHolder {
  FreeImageHolder(bool initContext)
    : ctx(initContext)
  {
    if (ctx)
    {
      FreeImage_Initialise();
    }
  }

  FIBITMAP* bmp = nullptr;
  FIMEMORY* mem = nullptr;
  bool ctx = false;

  ~FreeImageHolder()
  {
    FreeImage_Unload(bmp);
    FreeImage_CloseMemory(mem);
    if (ctx)
    {
      FreeImage_DeInitialise();
    }
  }
};

// nvtt handler
struct TPOutputHandler : public nvtt::OutputHandler {
  static const int32 MaxMipCount = 16;

  struct Mip {
    int32 SizeX = 0;
    int32 SizeY = 0;

    int32 Size = 0;
    int32 LastOffset = 0;
    void* Data = nullptr;
  };

  ~TPOutputHandler() override
  {
    for (int32 idx = 0; idx < MaxMipCount; ++idx)
    {
      free(Mips[idx].Data);
    }
  }

  void beginImage(int size, int width, int height, int depth, int face, int miplevel) override
  {
    if (miplevel >= MaxMipCount)
    {
      Ok = false;
      return;
    }
    Mip& mip = Mips[MipsCount];
    mip.Size = size;
    mip.SizeX = width;
    mip.SizeY = height;
    mip.Data = malloc(size);
    DBreakIf(depth != 1);
    MipsCount++;
  }

  bool writeData(const void* data, int size) override
  {
    if (!Ok)
    {
      return Ok;
    }
    Mip& mip = Mips[MipsCount - 1];
    if (mip.LastOffset + size > mip.Size)
    {
      if (void* tmp = realloc(mip.Data, mip.LastOffset + size))
      {
        mip.Size = mip.LastOffset + size;
        mip.Data = tmp;
      }
      else
      {
        Ok = false;
      }
    }
    if (Ok)
    {
      memcpy((char*)mip.Data + mip.LastOffset, data, size);
      mip.LastOffset += size;
    }
    return Ok;
  }

  virtual void endImage() override
  {}

  Mip Mips[MaxMipCount];
  int32 MipsCount = 0;
  bool Ok = true;
};

bool TextureProcessor::Process()
{
  if (InputPath.empty())
  {
    if (!InputData || !InputDataSize)
    {
      Error = "Texture Processor: no input data";
      return false;
    }
    
    if (OutputPath.empty())
    {
      if (!OutputData || !OutputDataSize)
      {
        Error = "Texture Processor: no output specified";
        return false;
      }
      return BytesToBytes();
    }
    return BytesToFile();
  }
  bool result = false;
  return result;
}

bool TextureProcessor::BytesToFile()
{
  nvtt::Surface surface;
  bool hasAlpha = true ;
  if (InputFormat == TCFormat::DXT1 || InputFormat == TCFormat::DXT3 || InputFormat == TCFormat::DXT5)
  {
    nvtt::Format fmt = nvtt::Format_Count;
    if (InputFormat == TCFormat::DXT1)
    {
      fmt = nvtt::Format_DXT1;
      hasAlpha = false;
    }
    else if (InputFormat == TCFormat::DXT3)
    {
      fmt = nvtt::Format_DXT3;
    }
    else if (InputFormat == TCFormat::DXT5)
    {
      fmt = nvtt::Format_DXT5;
    }
    if (!surface.setImage2D(fmt, nvtt::Decoder_D3D10, InputDataSizeX, InputDataSizeY, InputData))
    {
      Error = "Texture Processor: failed to create input surface (";
      Error += "DXT1:" + std::to_string(InputDataSizeX) + "x" + std::to_string(InputDataSizeY) + ")";
      return false;
    }
  }
  else if (InputFormat == TCFormat::RGBA8)
  {
    if (!surface.setImage(nvtt::InputFormat_BGRA_8UB, InputDataSizeX, InputDataSizeY, 1, InputData))
    {
      Error = "Texture Processor: failed to create input surface (";
      Error += "RGBA8:" + std::to_string(InputDataSizeX) + "x" + std::to_string(InputDataSizeY) + ")";
      return false;
    }
  }
  else
  {
    // TODO: export G8 as a luminance R8
    // TODO: export A8R8G8B8
    Error = "Texture Processor: unsupported input " + std::to_string((int)InputFormat);
    return false;
  }

  if (OutputFormat == TCFormat::TGA)
  {
    // TODO: nvtt doesn't support unicode paths. Decompress to float and save using FreeImage & FStream.
    if (!surface.save(OutputPath.c_str(), hasAlpha))
    {
      Error = "Texture Processor: failed to write the file \"" + OutputPath + "\"";
      return false;
    }
    return true;
  }

  if (OutputFormat == TCFormat::PNG)
  {
    surface.flipY();
    nvtt::CompressionOptions compressionOptions;
    compressionOptions.setFormat(hasAlpha ? nvtt::Format_RGBA : nvtt::Format_RGB);
    compressionOptions.setPixelFormat(hasAlpha ? 32 : 24, 0xFF0000, 0xFF00, 0xFF, hasAlpha ? 0xFF000000 : 0);

    nvtt::OutputOptions outputOptions;
    outputOptions.setSrgbFlag(SRGB);

    TPOutputHandler ohandler;
    outputOptions.setOutputHandler(&ohandler);

    nvtt::Context ctx;
    if (!ctx.compress(surface, 0, 0, compressionOptions, outputOptions))
    {
      Error = "Texture Processor: Failed to decompress texture to RGBA";
      return false;
    }

    FreeImageHolder holder(true);
    holder.bmp = FreeImage_Allocate(ohandler.Mips[0].SizeX, ohandler.Mips[0].SizeY, hasAlpha ? 32 : 24);
    memcpy(FreeImage_GetBits(holder.bmp), ohandler.Mips[0].Data, ohandler.Mips[0].Size);
    holder.mem = FreeImage_OpenMemory();

    if (!FreeImage_SaveToMemory(FIF_PNG, holder.bmp, holder.mem, 0))
    {
      Error = "Texture Processor: Failed to create a FreeImage(PNG:" + std::to_string(hasAlpha ? 32 : 24) + ")";
      return false;
    }

    DWORD memBufferSize = 0;
    unsigned char* memBuffer = nullptr;
    if (!FreeImage_AcquireMemory(holder.mem, &memBuffer, &memBufferSize))
    {
      Error = "Texture Processor: Failed to acquire a memory buffer";
      return false;
    }

    FWriteStream s(OutputPath);
    if (!s.IsGood())
    {
      Error = "Texture Processor: Failed to create a write stream to \"" + OutputPath + "\"";
      return false;
    }
    s.SerializeBytes(memBuffer, (FILE_OFFSET)memBufferSize);
    return true;
  }
  
  Error = "Texture Processor: Unsupported IO combination BytesToFile(\"" + std::to_string((int)InputFormat) + "\"" + std::to_string((int)InputFormat) + ")";
  return false;
}

bool TextureProcessor::BytesToBytes()
{
  return false;
}
