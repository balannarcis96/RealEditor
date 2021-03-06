#pragma once
#include "Core.h"
#include "FName.h"
#include "FStream.h"
#include "FString.h"
#include "FStructs.h"
#include "FName.h"

struct FPropertyTag;
class UField;
class UProperty;
struct FPropertyValue {

	FPropertyValue()
	{}

	FPropertyValue(FPropertyTag* property)
		: Property(property)
	{}

	FPropertyValue(FPropertyTag* property, UField* field)
		: Property(property)
		, Field(field)
	{}

	enum class VID
	{
		Unk,
		None,
		Bool,
		Byte,
		Int,
		Float,
		Object,
		Delegate,
		Property,
		Name,
		String,
		Struct,
		Field,
		Array,
	};

	inline bool* GetBoolPtr()
	{
		return (bool*)Data;
	}

	inline uint8* GetBytePtr()
	{
		return (uint8*)Data;
	}

	inline uint32* GetIntPtr()
	{
		return (uint32*)Data;
	}

	inline float* GetFloatPtr()
	{
		return (float*)Data;
	}

	inline FName* GetNamePtr()
	{
		return (FName*)Data;
	}

	inline FString* GetStringPtr()
	{
		return (FString*)Data;
	}

	inline PACKAGE_INDEX* GetObjectIdxPtr()
	{
		return (PACKAGE_INDEX*)Data;
	}

	inline FScriptDelegate* GetScriptDelegatePtr()
	{
		return (FScriptDelegate*)Data;
	}

	inline FPropertyTag* GetPropertyTagPtr()
	{
		return (FPropertyTag*)Data;
	}

	inline std::vector<FPropertyValue*>* GetArrayPtr()
	{
		return (std::vector<FPropertyValue*>*)Data;
	}

	inline bool& GetBool()
	{
		return *(bool*)Data;
	}

	inline uint8& GetByte()
	{
		return *(uint8*)Data;
	}

	inline uint32& GetInt()
	{
		return *(uint32*)Data;
	}

	inline float& GetFloat()
	{
		return *(float*)Data;
	}

	inline FName& GetName()
	{
		return *(FName*)Data;
	}

	inline FString& GetString()
	{
		return *(FString*)Data;
	}

	inline PACKAGE_INDEX& GetObjectIndex()
	{
		return *(PACKAGE_INDEX*)Data;
	}

	inline FScriptDelegate& GetScriptDelegate()
	{
		return *(FScriptDelegate*)Data;
	}

	inline FPropertyTag& GetPropertyTag()
	{
		return *(FPropertyTag*)Data;
	}

	inline std::vector<FPropertyValue*>& GetArray()
	{
		return *(std::vector<FPropertyValue*>*)Data;
	}

	~FPropertyValue();

	VID Type = VID::None;
	void* Data = nullptr;
	UEnum* Enum = nullptr;
	UField* Field = nullptr;
	UStruct* Struct = nullptr;
	FPropertyTag* Property = nullptr;
};

struct FPropertyTag {
	FName Type;
	uint8 BoolVal = 0;
	FName Name;
	FName StructName;
	FName EnumName;
	int32 Size = 0;
	int32 ArrayIndex = 0;
	int32 ArrayDim = 1;
	int32 SizeOffset = 0;
	
	UObject* Owner = nullptr;
	FPropertyValue* Value = nullptr;
	FPropertyTag* StaticArrayNext = nullptr;
	UProperty* ClassProperty = nullptr;

	FPropertyTag()
	{
		NewValue();
	}

	FPropertyTag(UObject* owner)
	{
		Owner = owner;
		NewValue();
	}

	~FPropertyTag()
	{
		if (Value)
		{
			delete Value;
		}
	}

	void NewValue()
	{
		if (Value)
		{
			delete Value;
		}
		Value = new FPropertyValue(this);
	}

	void* GetValueData()
	{
		return Value ? Value->Data : nullptr;
	}

	friend FStream& operator<<(FStream& s, FPropertyTag& tag)
	{
		s << tag.Name;
		if (tag.Name == NAME_None)
		{
			return s;
		}

		s << tag.Type;
		if (!s.IsReading())
		{
			tag.SizeOffset = s.GetPosition();
		}
		s << tag.Size << tag.ArrayIndex;

		if (tag.Type == NAME_StructProperty)
		{
			s << tag.StructName;
		}
		else if (tag.Type == NAME_BoolProperty)
		{
			if (s.GetFV() < VER_TERA_MODERN)
			{
				bool Value = tag.BoolVal;
				s << Value;
				tag.BoolVal = uint8(Value);
			}
			else
			{
				s << tag.BoolVal;
			}
		}
		else if (s.GetFV() > VER_TERA_CLASSIC && tag.Type == NAME_ByteProperty)
		{
			s << tag.EnumName;
		}
		return s;
	}
};