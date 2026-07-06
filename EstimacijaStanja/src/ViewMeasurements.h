// ViewMeasurements.h : editabilna "tabela" (mreza NumericEdit polja) za izmjenu
// vrijednosti PMU fazorskih mjerenja (z_re, z_im, sigma) - sadrzaj prikazan unutar
// MeasurementsDialog. Broj mjerenja nije fiksan na malom broju (kao za 3-cvorni slucaj)
// pa se kontrole prave dinamicki (std::vector<unique_ptr<...>>) umjesto fiksnih nizova -
// adrese objekata ostaju stabilne jer se nikad ne realociraju nakon konstrukcije.
#pragma once
#include <gui/View.h>
#include <gui/Label.h>
#include <gui/NumericEdit.h>
#include <gui/GridLayout.h>
#include <gui/GridComposer.h>
#include <td/String.h>
#include <td/Types.h>

#include <vector>
#include <memory>
#include <algorithm>

#include "EstimatorCore.h"

class ViewMeasurements : public gui::View
{
protected:
    gui::Label _lblColMeas;
    gui::Label _lblColZre;
    gui::Label _lblColZim;
    gui::Label _lblColSigma;

    std::vector<std::unique_ptr<gui::Label>> _lblDesc;
    std::vector<std::unique_ptr<gui::NumericEdit>> _neZre;
    std::vector<std::unique_ptr<gui::NumericEdit>> _neZim;
    std::vector<std::unique_ptr<gui::NumericEdit>> _neSigma;

    gui::GridLayout _gl;

    std::vector<se::PmuMeasurement>* _pMeas = nullptr;

    static td::String describe(const se::PmuMeasurement& m)
    {
        td::String s;
        const char* tip = (m.type == se::PmuMeasType::Voltage) ? "Vfazor" : "Ifazor";
        if (m.busJ >= 0)
            s.format("%s  bus %d->%d", tip, m.busI + 1, m.busJ + 1);
        else
            s.format("%s  bus %d", tip, m.busI + 1);
        return s;
    }

public:
    explicit ViewMeasurements(int nMeas)
    : _lblColMeas("Mjerenje")
    , _lblColZre("z (Re)")
    , _lblColZim("z (Im)")
    , _lblColSigma("sigma")
    , _gl(nMeas + 1, 4)
    {
        _lblDesc.reserve(nMeas);
        _neZre.reserve(nMeas);
        _neZim.reserve(nMeas);
        _neSigma.reserve(nMeas);

        for (int i = 0; i < nMeas; ++i)
        {
            _lblDesc.push_back(std::make_unique<gui::Label>(""));
            _neZre.push_back(std::make_unique<gui::NumericEdit>(td::real8));
            _neZim.push_back(std::make_unique<gui::NumericEdit>(td::real8));
            _neSigma.push_back(std::make_unique<gui::NumericEdit>(td::real8));

            _neZre.back()->setNumberOfDigitsAfterDecimalPoint(6);
            _neZim.back()->setNumberOfDigitsAfterDecimalPoint(6);
            _neSigma.back()->setNumberOfDigitsAfterDecimalPoint(6);
        }

        gui::GridComposer gc(_gl);
        gc.appendRow(_lblColMeas) << _lblColZre << _lblColZim << _lblColSigma;
        for (int i = 0; i < nMeas; ++i)
            gc.appendRow(*_lblDesc[i]) << *_neZre[i] << *_neZim[i] << *_neSigma[i];

        setLayout(&_gl);
    }

    void setMeasurements(std::vector<se::PmuMeasurement>* pMeas)
    {
        _pMeas = pMeas;
        if (!_pMeas)
            return;

        int n = std::min((int) _pMeas->size(), (int) _lblDesc.size());
        for (int i = 0; i < n; ++i)
        {
            const auto& m = (*_pMeas)[i];
            _lblDesc[i]->setTitle(describe(m));
            _neZre[i]->setValue(m.z.real());
            _neZim[i]->setValue(m.z.imag());
            _neSigma[i]->setValue(m.sigma);
        }
    }

    void applyToMeasurements()
    {
        if (!_pMeas)
            return;

        int n = std::min((int) _pMeas->size(), (int) _lblDesc.size());
        for (int i = 0; i < n; ++i)
        {
            double zre = 0.0, zim = 0.0, sigma = 0.0;
            _neZre[i]->getValue(zre);
            _neZim[i]->getValue(zim);
            _neSigma[i]->getValue(sigma);
            (*_pMeas)[i].z = se::cmplx(zre, zim);
            (*_pMeas)[i].sigma = sigma;
        }
    }
};
