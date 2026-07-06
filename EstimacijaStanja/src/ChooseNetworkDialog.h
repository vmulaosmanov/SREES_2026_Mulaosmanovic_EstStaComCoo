// ChooseNetworkDialog.h : mali dijalog koji se pojavi na klik "Ucitaj sistem" - bira se
// koja mreza da se ucita/generise (IEEE-14 ili IEEE-3, MATPOWER case3bus_P6_6).
#pragma once
#include <gui/Dialog.h>
#include <gui/View.h>
#include <gui/Label.h>
#include <gui/VerticalLayout.h>

#include "EstimatorCore.h"

class ChooseNetworkView : public gui::View
{
protected:
    gui::Label _lbl;
    gui::VerticalLayout _vl;

public:
    ChooseNetworkView()
    : _lbl("Izaberi mrezu za ucitavanje:")
    , _vl(1)
    {
        _vl << _lbl;
        setLayout(&_vl);
    }
};

class ChooseNetworkDialog : public gui::Dialog
{
protected:
    ChooseNetworkView _view;
    se::NetworkType _choice = se::NetworkType::IEEE14;

    bool onClick(Dialog::Button::ID btnID, gui::Button* pButton) override
    {
        if (btnID == Dialog::Button::ID::User0)
            _choice = se::NetworkType::IEEE14;
        else if (btnID == Dialog::Button::ID::User1)
            _choice = se::NetworkType::IEEE3;
        return true;
    }

public:
    ChooseNetworkDialog(gui::Frame* pFrame, td::UINT4 wndID = 0)
    : gui::Dialog(pFrame, { {gui::Dialog::Button::ID::User0, "IEEE-14", gui::Button::Type::Default},
                            {gui::Dialog::Button::ID::User1, "IEEE-3"},
                            {gui::Dialog::Button::ID::Cancel, "Odustani"} },
                  gui::Size(380, 130), wndID)
    {
        setTitle("Ucitaj sistem");
        setCentralView(&_view);
    }

    se::NetworkType getChoice() const { return _choice; }
};
