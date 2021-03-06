#pragma once
#include <wx/wx.h>
#include <Tera/FPropertyTag.h>

enum ToolEventID : int {
  eID_Export = 0,
  eID_Import,
  eID_Texture2D_Channel_R,
  eID_Texture2D_Channel_G,
  eID_Texture2D_Channel_B,
  eID_Texture2D_Channel_A
};

class UObject;
class PackageWindow;
class GenericEditor : public wxPanel
{
public:
  static GenericEditor* CreateEditor(wxPanel *parent, PackageWindow* window, UObject* object);
  GenericEditor(wxPanel* parent, PackageWindow* window);

  virtual void LoadObject();

  virtual void OnObjectLoaded();
  
  virtual UObject* GetObject()
  {
    return Object;
  }

  std::string GetEditorId() const;

  inline bool IsLoading() const
  {
    return Loading;
  }

  virtual void OnTick()
  {}

  virtual std::vector<FPropertyTag*> GetObjectProperties();

  virtual void PopulateToolBar(wxToolBar* toolbar);
  
  virtual void OnToolBarEvent(wxCommandEvent& e);

  virtual void OnExportClicked(wxCommandEvent& e);

  virtual void OnImportClicked(wxCommandEvent& e);

protected:
  virtual void OnObjectSet()
  {
  }

  void SetObject(UObject* object)
  {
    if ((Object = object))
    {
      OnObjectSet();
    }
  }

protected:
  UObject* Object = nullptr;
  PackageWindow* Window = nullptr;
  bool Loading = false;
};