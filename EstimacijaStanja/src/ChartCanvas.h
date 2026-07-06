// ChartCanvas.h : crta dva grafa jedan ispod drugog - magnituda napona (gore) i faza
// (dole), svaki sa biracem jedinica (magnituda: Volti/po jedinici; faza: po jedinici
// [dio kruga]/stepeni/radijani). Slika IEEE-14 se prikazuje odvojeno preko gui::ImageView
// (ne crta se ovdje) - vidi MainView.h.
#pragma once
#include <gui/Canvas.h>
#include <gui/Shape.h>
#include <gui/DrawableString.h>
#include <gui/Font.h>
#include <td/ColorID.h>

#include <vector>
#include <cmath>
#include <cstdio>
#include <algorithm>

#include "EstimatorCore.h"

class ChartCanvas : public gui::Canvas
{
public:
    enum class MagUnit { PerUnit, Volts };
    enum class PhaseUnit { PerUnit, Degrees, Radians };

    // Pretpostavljeni nazivni napon (koristi se samo za pretvaranje pu -> V) - IEEE-14
    // se u literaturi obicno navodi na ovom naponskom nivou.
    static constexpr double BASE_KV = 132.0;

protected:
    std::vector<se::cmplx> _V;
    bool _hasData = false;
    MagUnit _magUnit = MagUnit::PerUnit;
    PhaseUnit _phaseUnit = PhaseUnit::Degrees;

    double magValue(int i) const
    {
        double m = std::abs(_V[i]);
        return (_magUnit == MagUnit::Volts) ? (m * BASE_KV * 1000.0) : m;
    }

    double phaseValue(int i) const
    {
        double a = std::arg(_V[i]);   // radijani
        switch (_phaseUnit)
        {
        case PhaseUnit::Degrees: return a * 180.0 / se::PI;
        case PhaseUnit::PerUnit: return a / (2.0 * se::PI);   // dio pune kruzne rotacije
        case PhaseUnit::Radians:
        default:                 return a;
        }
    }

    // Nacrta jedan graf (tacke+linija) u zadanom pravougaoniku, sa horizontalnim
    // linijama mreze i oznakama y-vrijednosti; vraca (yMin,yMax) koji su koristeni.
    void drawSubChart(const gui::Rect& area, const char* title,
                       double (ChartCanvas::*valueFn)(int) const, int nGrid, const char* fmt)
    {
        const double left = 60, right = 20, top = 22, bottom = 22;
        gui::Point origin(area.left + left, area.bottom - bottom);
        double plotW = (area.right - area.left) - left - right;
        double plotTopY = area.top + top;
        double plotH = origin.y - plotTopY;
        if (plotW < 10 || plotH < 10)
            return;

        gui::DrawableString::draw(title, gui::Point(area.left + left, area.top + 2),
                                   gui::Font::ID::SystemBold, td::ColorID::SysText);

        if (!_hasData || _V.empty())
        {
            gui::DrawableString::draw("(nema podataka - klikni 'Estimiraj')",
                                       gui::Point(area.left + left, plotTopY + plotH / 2),
                                       gui::Font::ID::SystemNormal, td::ColorID::Gray);
            return;
        }

        int n = (int) _V.size();
        std::vector<double> vals(n);
        double vMin = 1e300, vMax = -1e300;
        for (int i = 0; i < n; ++i)
        {
            vals[i] = (this->*valueFn)(i);
            vMin = std::min(vMin, vals[i]);
            vMax = std::max(vMax, vals[i]);
        }
        if (vMax - vMin < 1e-9)
        {
            vMin -= 1.0;
            vMax += 1.0;
        }
        double pad = (vMax - vMin) * 0.15;
        double yMin = vMin - pad, yMax = vMax + pad;

        for (int g = 0; g <= nGrid; ++g)
        {
            double yv = yMin + (yMax - yMin) * g / nGrid;
            double py = origin.y - (yv - yMin) / (yMax - yMin) * plotH;
            gui::Shape::drawLine(gui::Point(origin.x, py), gui::Point(origin.x + plotW, py), td::ColorID::LightGray, 1);
            char buf[24];
            snprintf(buf, sizeof(buf), fmt, yv);
            gui::DrawableString::draw(buf, gui::Point(area.left + 4, py - 7), gui::Font::ID::SystemSmaller, td::ColorID::SysText);
        }

        gui::Shape::drawLine(origin, gui::Point(origin.x + plotW, origin.y), td::ColorID::Black, 2);
        gui::Shape::drawLine(origin, gui::Point(origin.x, plotTopY), td::ColorID::Black, 2);

        std::vector<gui::Point> pts(n);
        for (int i = 0; i < n; ++i)
        {
            double xFrac = (n == 1) ? 0.5 : (double) i / (double) (n - 1);
            double px = origin.x + xFrac * plotW;
            double py = origin.y - (vals[i] - yMin) / (yMax - yMin) * plotH;
            pts[i] = gui::Point(px, py);

            char buf[8];
            snprintf(buf, sizeof(buf), "%d", i + 1);
            gui::DrawableString::draw(buf, gui::Point(px - 4, origin.y + 4), gui::Font::ID::SystemSmaller, td::ColorID::SysText);
        }

        for (int i = 0; i + 1 < n; ++i)
            gui::Shape::drawLine(pts[i], pts[i + 1], td::ColorID::Blue, 2);

        for (int i = 0; i < n; ++i)
        {
            gui::Rect dot(pts[i].x - 3, pts[i].y - 3, pts[i].x + 3, pts[i].y + 3);
            gui::Shape::drawRect(dot, td::ColorID::Red);
        }
    }

    void onDraw(const gui::Rect& rDraw) override
    {
        gui::Size sz;
        getSize(sz);
        if (sz.width < 20 || sz.height < 40)
            return;

        double halfH = sz.height / 2.0;
        gui::Rect topArea(0, 0, sz.width, halfH);
        gui::Rect botArea(0, halfH, sz.width, sz.height);

        const char* magFmt = (_magUnit == MagUnit::Volts) ? "%.0f" : "%.3f";
        td::String magTitle;
        magTitle.format("Magnituda napona [%s]", (_magUnit == MagUnit::Volts) ? "V" : "p.u.");
        drawSubChart(topArea, magTitle.c_str(), &ChartCanvas::magValue, 4, magFmt);

        td::String phTitle;
        const char* phUnitStr = (_phaseUnit == PhaseUnit::Degrees) ? "deg" : (_phaseUnit == PhaseUnit::Radians) ? "rad" : "p.u.";
        phTitle.format("Faza napona [%s]", phUnitStr);
        drawSubChart(botArea, phTitle.c_str(), &ChartCanvas::phaseValue, 4, "%.3f");
    }

public:
    ChartCanvas() : gui::Canvas() {}

    void setVoltages(const std::vector<se::cmplx>& V)
    {
        _V = V;
        _hasData = true;
        reDraw();
    }

    void clearVoltages()
    {
        _hasData = false;
        _V.clear();
        reDraw();
    }

    void setMagUnit(MagUnit u) { _magUnit = u; reDraw(); }
    void setPhaseUnit(PhaseUnit u) { _phaseUnit = u; reDraw(); }
};
