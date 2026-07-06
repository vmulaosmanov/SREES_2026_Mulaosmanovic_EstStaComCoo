// MeasurementsDialog.h : modalni dijalog ("Izmjeni podatke") sa editabilnom tabelom
// PMU fazorskih mjerenja (ViewMeasurements). Na "Sacuvaj" (OK) primjenjuje izmjene
// direktno na vektor mjerenja koji drzi pozivalac (ControlsView).
#pragma once
#include <gui/Dialog.h>
#include "ViewMeasurements.h"

class MeasurementsDialog : public gui::Dialog
{
protected:
    ViewMeasurements _view;

    bool onClick(Dialog::Button::ID btnID, gui::Button* pButton) override
    {
        if (btnID == Dialog::Button::ID::OK)
            _view.applyToMeasurements();
        return true;
    }

public:
    MeasurementsDialog(gui::Frame* pFrame, std::vector<se::PmuMeasurement>* pMeas, td::UINT4 wndID = 0)
    : gui::Dialog(pFrame, { {gui::Dialog::Button::ID::OK, "Sacuvaj", gui::Button::Type::Default},
                            {gui::Dialog::Button::ID::Cancel, "Odustani"} },
                  gui::Size(560, 760), wndID)
    , _view((int) pMeas->size())
    {
        setTitle("Izmjena PMU fazorskih mjerenja");
        _view.setMeasurements(pMeas);
        setCentralView(&_view);
    }
};
