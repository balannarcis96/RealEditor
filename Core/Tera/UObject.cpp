#include "UObject.h"
#include "FStream.h"
#include "FPackage.h"
#include "FObjectResource.h"
#include "UClass.h"
#include "UComponent.h"

#include "ALog.h"

#if DUMP_OBJECTS
#include <filesystem>
#endif

UObject::UObject(FObjectExport* exp)
  : Export(exp)
{
#ifdef _DEBUG
  Description = Export->GetObjectName();
#endif
}

uint32 UObject::GetExportFlags() const
{
  return Export->ExportFlags;
}

uint64 UObject::GetObjectFlags() const
{
  return Export->ObjectFlags;
}

FString UObject::GetFullObjectName() const
{
  return Export->GetFullObjectName();
}

FPackage* UObject::GetPackage() const
{
  return Export->Package;
}

inline bool UObject::HasAnyFlags(uint64 flags) const
{
  return (GetObjectFlags() & flags) != 0 || flags == RF_AllFlags;
}

FILE_OFFSET UObject::GetSerialOffset() const
{
  return Export ? Export->SerialOffset : -1;
}

FILE_OFFSET UObject::GetSerialSize() const
{
  return Export ? Export->SerialSize : -1;
}

FILE_OFFSET UObject::GetPropertiesSize() const
{
  return RawDataOffset ? RawDataOffset - GetSerialOffset() : -1;
}

FILE_OFFSET UObject::GetDataSize() const
{
  return RawDataOffset ? (GetSerialOffset() + GetSerialSize() - RawDataOffset) : -1;
}

void* UObject::GetRawData()
{
  if (RawData)
  {
    return RawData;
  }
  FReadStream s(GetPackage()->GetDataPath());
  if (!RawDataOffset)
  {
    try
    {
      Load(s);
    }
    catch (const std::exception& e)
    {
      LogE("Failed to load the object %s", GetObjectName().C_str());
      LogE(e.what());
    }
    return nullptr;
  }
  if (!RawDataOffset || GetDataSize() <= 0)
  {
    return nullptr;
  }
  void* result = malloc(GetDataSize());
  s.SetPosition(RawDataOffset);
  s.SerializeBytes(result, GetDataSize());
  RawData = (char*)result;
  return result;
}

void UObject::SerializeScriptProperties(FStream& s) const
{
  // TODO: add a way to serialize object with a different package version
  if (s.GetFV() != FPackage::GetCoreVersion())
  {
    LogE("Can't read the object. Package %s was serialized with a different engine version", s.GetPackage()->GetPackageName().C_str());
    return;
  }
  if (Class)
  {
    Class->SerializeTaggedProperties(s, (UObject*)this, nullptr, HasAnyFlags(RF_ClassDefaultObject) ? Class->GetSuperClass() : Class, nullptr);
    return;
  }
  FName noneProp;
  s << noneProp;
  // TODO: serialize props without a Class obj
  //DBreakIf(nonePropertyName.String() != "None");
}

bool UObject::RegisterProperty(FPropertyTag* property)
{
  return false;
}

FString UObject::GetObjectPath() const
{
  FString path;
  if (FObjectResource* outer = Export->GetOuter())
  {
    path += outer->GetObjectPath();
  }
  path += "." + GetObjectName();
  return path;
}

FString UObject::GetObjectName() const
{
  return Export->GetObjectName();
}

FString UObject::GetClassName() const
{
  return Export->GetClassName();
}

UObject::~UObject()
{
  if (StateFrame)
  {
    delete StateFrame;
  }
  if (RawData)
  {
    delete[] RawData;
  }
  for (FPropertyTag* tag : Properties)
  {
    delete tag;
  }
  Properties.clear();
}

void UObject::Serialize(FStream& s)
{
  if (HasAnyFlags(RF_HasStack))
  {
    if (s.IsReading())
    {
      StateFrame = new FStateFrame();
    }
    if (StateFrame)
    {
      s << *StateFrame;
    }
  }

  if (IsA(UComponent::StaticClassName()))
  {
    ((UComponent*)this)->PreSerialize(s);
  }

  s << NetIndex;

  if (s.IsReading() && NetIndex != INDEX_NONE)
  {
    GetPackage()->AddNetObject(this);
  }

  if (GetStaticClassName() != UClass::StaticClassName())
  {
    SerializeScriptProperties(s);
  }

#if _DEBUG
  FILE_OFFSET curPos = s.GetPosition();
  FILE_OFFSET fileEnd = Export->SerialOffset + Export->SerialSize;
  DBreakIf(fileEnd < curPos);
#endif

  if (s.IsReading())
  {
    RawDataOffset = s.GetPosition();
  }
  // Serialize RawData for unimplemented classes
  if (!strcmp(GetStaticClassName(), UObject::StaticClassName()))
  {
    if (s.IsReading() && Export->SerialOffset + Export->SerialSize >= RawDataOffset)
    {
      RawDataSize = Export->SerialOffset + Export->SerialSize - RawDataOffset;
      if (RawDataSize)
      {
        RawData = new char[RawDataSize];
      }
    }
    s.SerializeBytes(RawData, RawDataSize);
  }
}

void UObject::Load()
{
  if (!GetPackage()->GetStream().GetLoadSerializedObjects())
  {
    return;
  }
  // Create a new stream here. This allows safe multithreading
  FReadStream s = FReadStream(A2W(GetPackage()->GetDataPath()));
  s.SetPackage(GetPackage());
  s.SetLoadSerializedObjects(GetPackage()->GetStream().GetLoadSerializedObjects());

  Load(s);
}

void UObject::Load(FStream& s)
{
  if (Loaded || Loading)
  {
    return;
  }
  Loading = true;

  // Load object's class and a default object
  if (GetClassName() != UClass::StaticClassName())
  {
    Class = GetPackage()->LoadClass(Export->ClassIndex);
    bool isNative = HasAnyFlags(RF_Native);
    PACKAGE_INDEX outerIndex = Export->OuterIndex;
    while (outerIndex && !isNative)
    {
      FObjectExport* outer = GetPackage()->GetExportObject(outerIndex);
      isNative = outer->ObjectFlags & RF_Native;
      outerIndex = outer->OuterIndex;
    }
    if (!isNative)
    {
      if (Class && !HasAnyFlags(RF_ClassDefaultObject))
      {
        DefaultObject = Class->GetClassDefaultObject();
      }
    }
  }

  if (s.IsReading())
  {
    s.SetPosition(Export->SerialOffset);
#if DUMP_OBJECTS
    void* data = malloc(Export->SerialSize);
    s.SerializeBytes(data, Export->SerialSize);
    s.SetPosition(Export->SerialOffset);
    std::filesystem::path path = std::filesystem::path(DUMP_PATH) / GetPackage()->GetPackageName().String() / "Objects";
    std::filesystem::create_directories(path);
    path /= (Export->GetFullObjectName().String() + ".bin");
    std::ofstream os(path.wstring(), std::ios::out | std::ios::binary);
    os.write((const char*)data, Export->SerialSize);
    free(data);
#endif
  }
  
#if SERIALIZE_PROPERTIES
  if (HasAnyFlags(RF_ClassDefaultObject))
  {
    SerializeDefaultObject(s);
  }
  else
  {
    Serialize(s);
  }
#endif

  Loaded = true;
  Loading = false;

  if (s.IsReading())
  {
    //DBreakIf(s.GetPosition() != Export->SerialOffset + Export->SerialSize);
    PostLoad();
  }
}

void UObject::PostLoad()
{
}

void UObject::SerializeDefaultObject(FStream& s)
{
  s << NetIndex;
  if (s.IsReading() && NetIndex != INDEX_NONE)
  {
    GetPackage()->AddNetObject(this);
  }
  if (GetStaticClassName() != UClass::StaticClassName())
  {
    SerializeScriptProperties(s);
  }
}

bool UObject::IsA(const char* base) const
{
  const auto thisClassName = GetStaticClassName();
  if (base == thisClassName)
  {
    return true;
  }
  for (UClass* tmp = Class; tmp; tmp = static_cast<UClass*>(tmp->GetSuperStruct()))
  {
    if (base == tmp->GetStaticClassName())
    {
      return true;
    }
  }
  std::string chain = GetStaticClassChain();
  size_t pos = chain.size();
  size_t lastPos = pos;
  while ((pos = chain.find_last_of('.', pos)) != std::string::npos)
  {
    if (std::string_view(chain.c_str() + pos + 1, lastPos - pos - 1) == base)
    {
      return true;
    }
    lastPos = pos;
    pos--;
  }
  return false;
}

FStream& operator<<(FStream& s, UObject*& obj)
{
  PACKAGE_INDEX idx = 0;
  if (s.IsReading())
  {
    s << idx;
    FILE_OFFSET tmpPos = s.GetPosition();
    obj = s.GetPackage()->GetObject(idx, s.GetLoadSerializedObjects());
    s.SetPosition(tmpPos);
  }
  else
  {
    idx = s.GetPackage()->GetObjectIndex(obj);
    s << idx;
  }
  return s;
}
