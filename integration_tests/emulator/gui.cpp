//////////////////////////////////////////////////////////////////////
// This file was auto-generated by codelite's wxCrafter Plugin
// wxCrafter project file: gui.wxcp
// Do not modify this file by hand!
//////////////////////////////////////////////////////////////////////

#include "gui.h"


// Declare the bitmap loading function
extern void wxC6B49InitBitmapResources();

static bool bBitmapLoaded = false;


MainFrame::MainFrame(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style)
    : wxFrame(parent, id, title, pos, size, style)
{
    if ( !bBitmapLoaded ) {
        // We need to initialise the default bitmap handler
        wxXmlResource::Get()->AddHandler(new wxBitmapXmlHandler);
        wxC6B49InitBitmapResources();
        bBitmapLoaded = true;
    }
    
    wxGridSizer* gridSizer2 = new wxGridSizer(2, 2, 0, 0);
    this->SetSizer(gridSizer2);
    
    Close = new wxButton(this, closeButton, _("Close"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), 0);
    
    gridSizer2->Add(Close, 0, wxALL, WXC_FROM_DIP(5));
    
    m_button6 = new wxButton(this, wxID_ANY, _("My Button"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), 0);
    
    gridSizer2->Add(m_button6, 0, wxALL, WXC_FROM_DIP(5));
    
    m_button8 = new wxButton(this, wxID_ANY, _("My Button"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), 0);
    
    gridSizer2->Add(m_button8, 0, wxALL, WXC_FROM_DIP(5));
    
    m_button10 = new wxButton(this, wxID_ANY, _("My Button"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), 0);
    
    gridSizer2->Add(m_button10, 0, wxALL, WXC_FROM_DIP(5));
    
    SetName(wxT("MainFrame"));
    SetSize(500,300);
    if (GetSizer()) {
         GetSizer()->Fit(this);
    }
    if(GetParent()) {
        CentreOnParent(wxBOTH);
    } else {
        CentreOnScreen(wxBOTH);
    }
#if wxVERSION_NUMBER >= 2900
    if(!wxPersistenceManager::Get().Find(this)) {
        wxPersistenceManager::Get().RegisterAndRestore(this);
    } else {
        wxPersistenceManager::Get().Restore(this);
    }
#endif
}

MainFrame::~MainFrame()
{
}
