// NetworkDiagramCanvas.h : crta sematski dijagram mreze - cvorovi (kvadratici sa brojem
// busa), grane (linije) i smjer toka struje (strelica ciji smjer odredjuje predznak
// realne snage toka P_ij = Re(V_i*conj(I_ij)) - standardna inzenjerska konvencija).
// Napon u voltima se ispisuje iznad svakog cvora nakon estimacije. Smjer strelice se
// ponovo racuna iz TRENUTNOG procijenjenog napona svaki put kad se pozove setVoltages()
// (npr. nakon klika "Estimiraj"), pa se automatski mijenja ako se mjerenja promijene.
#pragma once
#include <gui/Canvas.h>
#include <gui/Shape.h>
#include <gui/DrawableString.h>
#include <gui/Font.h>
#include <td/ColorID.h>

#include <vector>
#include <utility>
#include <cmath>
#include <cstdio>

#include "EstimatorCore.h"
#include "SystemFile.h"

class NetworkDiagramCanvas : public gui::Canvas
{
protected:
    // Isti pretpostavljeni nazivni napon kao u ChartCanvas-u (namjerno lokalna kopija -
    // ChartCanvas.h se ne dira).
    static constexpr double BASE_KV = 132.0;

    std::vector<se::Branch> _branches;
    std::vector<std::pair<double, double>> _positions;   // normalizovano [0,1]x[0,1]
    std::vector<se::cmplx> _V;
    bool _hasData = false;

    gui::Point pixelPos(int busIdx, const gui::Size& sz, double marginX, double marginTop, double marginBottom) const
    {
        double nx = _positions[busIdx].first;
        double ny = _positions[busIdx].second;
        double px = marginX + nx * (sz.width - 2.0 * marginX);
        double py = marginTop + ny * (sz.height - marginTop - marginBottom);
        return gui::Point(px, py);
    }

    // Nacrta strelicu (dva kratka kraka) ciji vrh je na 60% puta od 'from' ka 'to' -
    // koristi iskljucivo gui::Shape::drawLine (staticka metoda, ista koja se vec koristi
    // u ChartCanvas-u), bez potrebe za Shape/Polygon objektima.
    void drawArrow(const gui::Point& from, const gui::Point& to) const
    {
        double dx = to.x - from.x, dy = to.y - from.y;
        double len = std::sqrt(dx * dx + dy * dy);
        if (len < 1e-6)
            return;

        double ux = dx / len, uy = dy / len;
        gui::Point tip(from.x + 0.6 * dx, from.y + 0.6 * dy);

        const double arrowLen = 11.0;
        const double ang = 0.45;   // ~26 stepeni razmak krakova strelice

        auto rot = [&](double cx, double cy, double a)
        {
            double c = std::cos(a), s = std::sin(a);
            return gui::Point(tip.x - arrowLen * (cx * c - cy * s), tip.y - arrowLen * (cx * s + cy * c));
        };

        gui::Point wing1 = rot(ux, uy, ang);
        gui::Point wing2 = rot(ux, uy, -ang);

        gui::Shape::drawLine(tip, wing1, td::ColorID::OrangeRed, 2);
        gui::Shape::drawLine(tip, wing2, td::ColorID::OrangeRed, 2);
    }

    void onDraw(const gui::Rect& rDraw) override
    {
        gui::Size sz;
        getSize(sz);
        if (sz.width < 20 || sz.height < 20)
            return;

        if (_positions.empty())
        {
            gui::DrawableString::draw("Klikni 'Ucitaj sistem' da vidis semu mreze.",
                                       gui::Point(20, sz.height / 2.0),
                                       gui::Font::ID::SystemNormal, td::ColorID::Gray);
            return;
        }

        const double marginX = 45, marginTop = 55, marginBottom = 40;

        // grane (linije + strelica smjera toka)
        for (const auto& br : _branches)
        {
            gui::Point p1 = pixelPos(br.i, sz, marginX, marginTop, marginBottom);
            gui::Point p2 = pixelPos(br.j, sz, marginX, marginTop, marginBottom);
            gui::Shape::drawLine(p1, p2, td::ColorID::Gray, 2);

            if (_hasData && _V.size() == _positions.size())
            {
                auto terms = se::branchCurrentTerms(br, br.i);
                se::cmplx I(0.0, 0.0);
                for (const auto& t : terms)
                    I += t.y * _V[t.bus];

                se::cmplx S = _V[br.i] * std::conj(I);   // snaga koja izlazi iz cvora br.i
                bool fromToJ = (S.real() >= 0.0);         // tok i->j ako je P>=0, inace j->i

                if (fromToJ)
                    drawArrow(p1, p2);
                else
                    drawArrow(p2, p1);
            }
        }

        // cvorovi (kvadratic + broj busa + napon iznad, ako postoji estimacija)
        for (size_t i = 0; i < _positions.size(); ++i)
        {
            gui::Point p = pixelPos((int) i, sz, marginX, marginTop, marginBottom);
            gui::Rect nodeRect(p.x - 12, p.y - 12, p.x + 12, p.y + 12);
            gui::Shape::drawRect(nodeRect, td::ColorID::White, td::ColorID::Black, 2.0f);

            char buf[8];
            snprintf(buf, sizeof(buf), "%d", (int) i + 1);
            gui::DrawableString::draw(buf, nodeRect, gui::Font::ID::SystemSmaller, td::ColorID::Black,
                                       td::TextAlignment::Center, td::VAlignment::Center);

            if (_hasData && i < _V.size())
            {
                double volts = std::abs(_V[i]) * BASE_KV * 1000.0;
                char vbuf[24];
                snprintf(vbuf, sizeof(vbuf), "%.0f V", volts);
                gui::DrawableString::draw(vbuf, gui::Point(p.x - 22, p.y - 30),
                                           gui::Font::ID::SystemSmaller, td::ColorID::SysText);
            }
        }
    }

public:
    NetworkDiagramCanvas() : gui::Canvas() {}

    void setNetwork(se::NetworkType type, const std::vector<se::Branch>& branches)
    {
        _branches = branches;
        _positions = se::networkNodePositions(type);
        _hasData = false;
        _V.clear();
        reDraw();
    }

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
};
