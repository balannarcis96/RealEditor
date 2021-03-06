#include "UTexture.h"
#include "Cast.h"
#include "FPackage.h"

#include "ALog.h"

struct PF_Info {
  uint32 BlockSizeX = 0;
  uint32 BlockSizeY = 0;
  uint32 BlockSizeZ = 0;
  uint32 BlockBytes = 0;
  uint32 NumComponents = 0;
  GLenum Format = 0;
};

PF_Info PixelFormatInfo[] = {
  //BsX     BsY     BsZ     Bb      Comps   GL_Format           //  Format
  { 0,			0,			0,			0,			0, 			GL_NONE},           //	PF_Unknown
  { 1,			1,			1,			16,			4, 			GL_RGBA},           //	PF_A32B32G32R32F
  { 1,			1,			1,			4,			4, 			GL_BGRA_EXT},       //	PF_A8R8G8B8
  { 1,			1,			1,			1,			1, 			GL_LUMINANCE},      //	PF_G8
  { 1,			1,			1,			2,			1, 			GL_NONE},           //	PF_G16
  { 4,			4,			1,			8,			3, 			GL_RGBA},           //	PF_DXT1
  { 4,			4,			1,			16,			4, 			GL_RGBA},           //	PF_DXT3
  { 4,			4,			1,			16,			4, 			GL_RGBA},           //	PF_DXT5
  { 2,			1,			1,			4,			4, 			GL_NONE},           //	PF_UYVY
  { 1,			1,			1,			8,			3, 			GL_RGB},            //	PF_FloatRGB
  { 1,			1,			1,			8,			4, 			GL_RGBA},           //	PF_FloatRGBA
  { 1,			1,			1,			0,			1, 			GL_DEPTH_STENCIL},  //	PF_DepthStencil
  { 1,			1,			1,			4,			1, 			GL_DEPTH_COMPONENT},//	PF_ShadowDepth
  { 1,			1,			1,			4,			1, 			GL_DEPTH_COMPONENT},//	PF_FilteredShadowDepth
  { 1,			1,			1,			4,			1, 			GL_RED},            //	PF_R32F
  { 1,			1,			1,			4,			2, 			GL_RG},             //	PF_G16R16
  { 1,			1,			1,			4,			2, 			GL_RG},             //	PF_G16R16F
  { 1,			1,			1,			4,			2, 			GL_RG},             //	PF_G16R16F_FILTER
  { 1,			1,			1,			8,			2, 			GL_RG},             //	PF_G32R32F
  { 1,			1,			1,			4,			4, 			GL_RGBA},           //  PF_A2B10G10R10
  { 1,			1,			1,			8,			4, 			GL_RGBA},           //	PF_A16B16G16R16
  { 1,			1,			1,			4,			1, 			GL_NONE},           //	PF_D24
  { 1,			1,			1,			2,			1, 			GL_RED},            //	PF_R16F
  { 1,			1,			1,			2,			1, 			GL_RED},            //	PF_R16F_FILTER
  { 4,			4,			1,			16,			2, 			GL_NONE},           //	PF_BC5
  { 1,			1,			1,			2,			2, 			GL_RG},             //	PF_V8U8
  { 1,			1,			1,			1,			1, 			GL_NONE},           //	PF_A1			
  { 1,			1,			1,			0,			3, 			GL_NONE},           //	PF_FloatR11G11B10
};

inline bool FindInternalFormatAndType(uint32 pixelFormat, GLenum& outFormat, GLenum& outType, bool bSRGB)
{
  switch (pixelFormat)
  {
  case PF_A32B32G32R32F:
    outFormat = bSRGB ? GL_NONE : GL_RGBA32F_ARB;
    outType = GL_UNSIGNED_NORMALIZED_ARB;
    return true;
  case PF_A8R8G8B8:
    outFormat = bSRGB ? GL_SRGB8_ALPHA8_EXT : GL_RGBA8;
    outType = GL_UNSIGNED_INT_8_8_8_8_REV;
    return true;
  case PF_DXT1:
    outFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    outType = GL_UNSIGNED_BYTE;
    return true;
  case PF_DXT3:
    outFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    outType = GL_UNSIGNED_BYTE;
    return true;
  case PF_DXT5:
    outFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    outType = GL_UNSIGNED_BYTE;
    return true;
  case PF_G8:
    outFormat = bSRGB ? GL_RGB8 : GL_LUMINANCE8;
    outType = GL_UNSIGNED_BYTE;
    return true;
  case PF_DepthStencil:
    outFormat = bSRGB ? GL_NONE : GL_DEPTH24_STENCIL8_EXT;
    outType = GL_UNSIGNED_INT_24_8_EXT;
    return true;
  case PF_D24:
  case PF_ShadowDepth:
  case PF_FilteredShadowDepth:
    outFormat = bSRGB ? GL_NONE : GL_DEPTH_COMPONENT24_ARB;
    outType = GL_UNSIGNED_INT;
    return true;
  case PF_R32F:
    outFormat = bSRGB ? GL_NONE : GL_R32F;
    outType = GL_FLOAT;
    return true;
  case PF_G16R16:
    outFormat = bSRGB ? GL_RGBA16 : GL_RG16;
    outType = GL_UNSIGNED_SHORT;
    return true;
  case PF_G16R16F:
  case PF_G16R16F_FILTER:
    outFormat = bSRGB ? GL_RGBA16F_ARB : GL_RG16F;
    outType = GL_HALF_FLOAT_ARB;
    return true;
  case PF_G32R32F:
    outFormat = bSRGB ? GL_RGBA32F_ARB : GL_RG32F;
    outType = GL_FLOAT;
    return true;
  case PF_A2B10G10R10:
    outFormat = bSRGB ? GL_NONE : GL_RGB10_A2;
    outType = GL_UNSIGNED_INT_2_10_10_10_REV;
    return true;
  case PF_A16B16G16R16:
    outFormat = bSRGB ? GL_NONE : GL_RGBA16F_ARB;
    outType = GL_HALF_FLOAT_ARB;
    return true;
  case PF_R16F:
  case PF_R16F_FILTER:
    outFormat = bSRGB ? GL_NONE : GL_R16F;
    outType = GL_HALF_FLOAT_ARB;
    return true;
  case PF_FloatRGB:
    outFormat = bSRGB ? GL_NONE : GL_RGB16F_ARB;
    outType = GL_HALF_FLOAT_ARB;
    return true;
  case PF_FloatRGBA:
    outFormat = bSRGB ? GL_NONE : GL_RGBA16F_ARB;
    outType = GL_HALF_FLOAT_ARB;
    return true;
  case PF_V8U8:
    outFormat = bSRGB ? GL_NONE : GL_RG8;
    outType = GL_BYTE;
    return true;
  default:
    return false;
  }
}

bool UTexture::RegisterProperty(FPropertyTag* property)
{
  if (PROP_IS(property, SRGB))
  {
    SRGB = property->BoolVal;
    return true;
  }
  return false;
}

void UTexture::Serialize(FStream& s)
{
  Super::Serialize(s);
  SourceArt.Serialize(s, this);
}

UTexture2D::~UTexture2D()
{
  for (auto& e : Mips)
  {
    delete e;
  }
  for (auto& e : CachedMips)
  {
    delete e;
  }
  for (auto& e : CachedAtiMips)
  {
    delete e;
  }
  for (auto& e : CachedEtcMips)
  {
    delete e;
  }
}

bool UTexture2D::RegisterProperty(FPropertyTag* property)
{
  if (Super::RegisterProperty(property))
  {
    return false;
  }
  if (PROP_IS(property, Format))
  {
    Format = (EPixelFormat)property->Value->GetByte();
    return true;
  }
  else if (PROP_IS(property, SizeX))
  {
    SizeX = property->Value->GetInt();
    return true;
  }
  else if (PROP_IS(property, SizeY))
  {
    SizeY = property->Value->GetInt();
    return true;
  }
  else if (PROP_IS(property, AddressX))
  {
    AddressX = (TextureAddress)property->Value->GetByte();
    return true;
  }
  else if (PROP_IS(property, AddressY))
  {
    AddressY = (TextureAddress)property->Value->GetByte();
    return true;
  }
  else if (PROP_IS(property, LODGroup))
  {
    LODGroup = (TextureGroup)property->Value->GetByte();
    return true;
  }
  else if (PROP_IS(property, MipTailBaseIdx))
  {
    MipTailBaseIdx = property->Value->GetInt();
    return true;
  }
  else if (PROP_IS(property, bNoTiling))
  {
    bNoTiling = property->Value->GetBool();
    return true;
  }
  return false;
}

osg::ref_ptr<osg::Image> UTexture2D::GetTextureResource()
{
  if (TextureResource)
  {
    return TextureResource;
  }
  GLenum format = PixelFormatInfo[Format].Format;
  GLenum type = 0;
  if (!Mips.size() || !FindInternalFormatAndType(Format, format, type, SRGB) || format == GL_NONE)
  {
    return nullptr;
  }
  FTexture2DMipMap* mip = Mips.front();
  TextureResource = new osg::Image();
  TextureResource->setFileName(GetObjectName().String());
  TextureResource->setImage(mip->SizeX, mip->SizeY, 0, format, format, type, (uint8*)mip->Data.GetAllocation(), osg::Image::AllocationMode::NO_DELETE, 1, 0);
  return TextureResource;
}

void UTexture2D::Serialize(FStream& s)
{
  Super::Serialize(s);
  if (s.GetFV() == VER_TERA_CLASSIC || s.GetFV() == VER_TERA_MODERN)
  {
    s << SourceFilePath;
  }
  s.SerializeUntypeBulkDataArray(Mips, this);
  s << TextureFileCacheGuid;
  if (s.GetFV() >= VER_TERA_CLASSIC)
  {
    s.SerializeUntypeBulkDataArray(CachedMips, this);
    s << MaxCachedResolution;
    s.SerializeUntypeBulkDataArray(CachedAtiMips, this);
    CachedFlashMip.Serialize(s, this);
    s.SerializeUntypeBulkDataArray(CachedEtcMips, this);
  }
}

void UTexture2D::PostLoad()
{
  Super::PostLoad();
  FString cacheName;
  FStream* rs = nullptr;
  for (int32 idx = 0; idx < Mips.size(); ++idx)
  {
    FTexture2DMipMap* mip = Mips[idx];
    if (mip->Data.IsStoredInSeparateFile())
    {
      FString bulkDataName = GetObjectPath() + ".MipLevel_" + std::to_string(idx);
      bulkDataName = bulkDataName.ToUpper();
      FBulkDataInfo* info = FPackage::GetBulkDataInfo(bulkDataName);
      if (!info)
      {
        bulkDataName += "DXT";
        info = FPackage::GetBulkDataInfo(bulkDataName);
      }
      if (info)
      {
        if (cacheName != info->TextureFileCacheName)
        {
          if (rs)
          {
            delete rs;
          }
          FString path = FPackage::GetTextureFileCachePath(info->TextureFileCacheName);
          if (path.Size())
          {
            rs = new FReadStream(path);
            if (rs->IsGood())
            {
              cacheName = info->TextureFileCacheName;
            }
          }
        }
        if (rs)
        {
          FStream& s = *rs;
          s.SetPosition(info->SavedBulkDataOffsetInFile);
          try
          {
            mip->Data.SerializeSeparate(s, this, idx);
          }
          catch (...)
          {
            LogE("Failed to decompress bulkdata: %s", bulkDataName.C_str());
            delete rs;
            return;
          }
        }
      }
    }
  }
  if (rs)
  {
    delete rs;
  }
}


EPixelFormat UTexture2D::GetEffectivePixelFormat(const EPixelFormat format, bool bSRGB)
{
  if ((format == PF_G8) && bSRGB)
  {
    return PF_A8R8G8B8;
  }
  return format;
}