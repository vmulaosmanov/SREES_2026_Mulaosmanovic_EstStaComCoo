// MainView.h : jedan prozor bez tabova - lijevo dugmad (Ucitaj sistem / Izmjeni podatke /
// Estimiraj) jedno ispod drugog + status (donji lijevi kvadrat, visereda), desno
// sematski dijagram mreze (pojavi se nakon "Ucitaj sistem", sa smjerom toka struje
// nakon "Estimiraj"), birac jedinica i graf (magnituda + faza).
#pragma once
#include <gui/View.h>
#include <gui/Button.h>
#include <gui/TextEdit.h>
#include <gui/Label.h>
#include <gui/ComboBox.h>
#include <gui/HorizontalLayout.h>
#include <gui/VerticalLayout.h>
#include <gui/Alert.h>
#include <gui/Window.h>
#include <td/String.h>

#include <vector>

#include "EstimatorCore.h"
#include "SystemFile.h"
#include "ChartCanvas.h"
#include "NetworkDiagramCanvas.h"
#include "MeasurementsDialog.h"
#include "ChooseNetworkDialog.h"

class MainView : public gui::View
{
protected:
    se::NetworkType _network = se::NetworkType::IEEE14;
    std::vector<se::Branch> _branches;
    std::vector<se::PmuMeasurement> _meas;

    // ---- lijeva kolona: dugmad + status (donji lijevi kvadrat, moze u vise redova) ----
    gui::Button _btnLoadSystem;
    gui::Button _btnEditMeasurements;
    gui::Button _btnEstimate;
    gui::TextEdit _statusText;
    gui::VerticalLayout _leftCol;

    // ---- desna kolona: dijagram mreze + birac jedinica + graf ----
    NetworkDiagramCanvas _netDiagram;

    gui::Label _lblMagUnit;
    gui::ComboBox _cmbMagUnit;
    gui::Label _lblPhaseUnit;
    gui::ComboBox _cmbPhaseUnit;
    gui::HorizontalLayout _unitRow;

    ChartCanvas _chart;
    gui::VerticalLayout _rightCol;

    // ---- glavni (top-level) raspored: lijevo | desno ----
    gui::HorizontalLayout _topRow;

public:
    MainView()
    : _btnLoadSystem("Ucitaj sistem")
    , _btnEditMeasurements("Izmjeni podatke")
    , _btnEstimate("Estimiraj")
    , _statusText(gui::TextEdit::HorizontalScroll::No, gui::TextEdit::Events::DoNotSend, true)
    , _leftCol(4)
    , _lblMagUnit("Napon:")
    , _cmbMagUnit()
    , _lblPhaseUnit("Faza:")
    , _cmbPhaseUnit()
    , _unitRow(4)
    , _rightCol(3)
    , _topRow(2)
    {
        _btnEstimate.setAsDefault();

        _statusText.setText("IEEE-14/IEEE-3 sistem nije ucitan.");
        _statusText.setSizeLimits(150, gui::Control::Limit::UseAsMin, 55, gui::Control::Limit::Fixed);

        _leftCol.setSpaceBetweenCells(75);   // ~2cm razmaka izmedju dugmadi (i statusa)
        _leftCol << _btnLoadSystem << _btnEditMeasurements << _btnEstimate << _statusText;

        _netDiagram.setFixedHeight(280);

        _cmbMagUnit.addItem("Po jedinici (p.u.)");
        _cmbMagUnit.addItem("Volti (V)");
        _cmbMagUnit.selectIndex(0);
        _cmbMagUnit.onChangedSelection([this]()
        {
            _chart.setMagUnit(_cmbMagUnit.getSelectedIndex() == 1 ? ChartCanvas::MagUnit::Volts : ChartCanvas::MagUnit::PerUnit);
        });

        _cmbPhaseUnit.addItem("Po jedinici (dio kruga)");
        _cmbPhaseUnit.addItem("Stepeni");
        _cmbPhaseUnit.addItem("Radijani");
        _cmbPhaseUnit.selectIndex(1);
        _cmbPhaseUnit.onChangedSelection([this]()
        {
            int idx = _cmbPhaseUnit.getSelectedIndex();
            ChartCanvas::PhaseUnit u = (idx == 0) ? ChartCanvas::PhaseUnit::PerUnit
                                      : (idx == 2) ? ChartCanvas::PhaseUnit::Radians
                                                    : ChartCanvas::PhaseUnit::Degrees;
            _chart.setPhaseUnit(u);
        });
        _chart.setPhaseUnit(ChartCanvas::PhaseUnit::Degrees);

        _unitRow << _lblMagUnit << _cmbMagUnit << _lblPhaseUnit << _cmbPhaseUnit;

        _rightCol << _netDiagram;
        _rightCol.appendLayout(_unitRow);
        _rightCol << _chart;

        _topRow.appendLayout(_leftCol);
        _topRow.appendLayout(_rightCol);

        setLayout(&_topRow);
    }

    bool onClick(gui::Button* pBtn) override
    {
        if (pBtn == &_btnLoadSystem)
        {
            onLoadSystem();
            return true;
        }
        if (pBtn == &_btnEditMeasurements)
        {
            onEditMeasurements();
            return true;
        }
        if (pBtn == &_btnEstimate)
        {
            onEstimate();
            return true;
        }
        return false;
    }

protected:
    void onLoadSystem()
    {
        gui::Window* pParentWnd = getParentWindow();
        auto* pDlg = new ChooseNetworkDialog(pParentWnd);
        pDlg->openModal([this](gui::Dialog* pDlg)
        {
            if (pDlg->getClickedButtonID() == gui::Dialog::Button::ID::Cancel)
                return;

            auto* pChooseDlg = static_cast<ChooseNetworkDialog*>(pDlg);
            loadOrGenerateNetwork(pChooseDlg->getChoice());
        });
    }

    void loadOrGenerateNetwork(se::NetworkType type)
    {
        _network = type;
        const std::string fileName = se::systemFileName(type);

        if (!se::loadSystem(fileName, _branches, _meas))
        {
            se::generateExampleSystem(type, _branches, _meas);
            se::saveSystem(fileName, _branches, _meas);

            td::String s;
            s.format("Fajl '%s' nije postojao.\nGenerisan je %s primjer sistema (%d grana, %d PMU mjerenja) i sacuvan.",
                      fileName.c_str(), se::networkDisplayName(type), (int) _branches.size(), (int) _meas.size());
            _statusText.setText(s);
        }
        else
        {
            td::String s;
            s.format("Ucitano je: %d grana, %d PMU mjerenja\niz '%s' (%s).",
                      (int) _branches.size(), (int) _meas.size(), fileName.c_str(), se::networkDisplayName(type));
            _statusText.setText(s);
        }

        _netDiagram.setNetwork(type, _branches);
        _chart.clearVoltages();
    }

    void onEditMeasurements()
    {
        if (_meas.empty())
        {
            gui::Alert::show("Izmjena mjerenja", "Prvo ucitaj sistem (dugme 'Ucitaj sistem').");
            return;
        }

        gui::Window* pParentWnd = getParentWindow();
        auto* pDlg = new MeasurementsDialog(pParentWnd, &_meas);
        pDlg->openModal([this](gui::Dialog* pDlg)
        {
            if (pDlg->getClickedButtonID() == gui::Dialog::Button::ID::OK)
            {
                se::saveSystem(se::systemFileName(_network), _branches, _meas);
                _statusText.setText("Mjerenja azurirana i sacuvana.");
            }
        });
    }

    void onEstimate()
    {
        if (_branches.empty() || _meas.empty())
        {
            gui::Alert::show("Estimacija", "Prvo ucitaj sistem (dugme 'Ucitaj sistem').");
            return;
        }

        se::EstimationResult result = se::estimateStatePMU(_branches, _meas);

        if (result.singular)
        {
            gui::Alert::show("Estimacija", "Gain matrica je singularna u iteraciji - estimacija nije uspjela.");
            return;
        }

        _chart.setVoltages(result.V);
        _netDiagram.setVoltages(result.V);

        td::String s;
        s.format("Konvergiralo za %d iteracija\n(max|dV|=%.2e, %d PMU mjerenja,\n%d cvorova, %s).",
                  result.iterations, result.maxDx, (int) _meas.size(), (int) result.V.size(), se::networkDisplayName(_network));
        _statusText.setText(s);
    }
};
