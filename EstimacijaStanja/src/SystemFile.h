// SystemFile.h : ucitavanje/snimanje kombinovanog fajla (topologija mreze + PMU
// fazorska mjerenja) i generisanje pocetnog primjera kad fajl jos ne postoji.
// Podrzane su dvije mreze (bira ih korisnik u dijalogu "Ucitaj sistem"):
//   - IEEE-14 standardni test sistem (busdata/linedata isti kao u referentnom MATLAB
//     "busdatas.m"/"linedatas.m"), PMU na busevima {2,6,7,9} (1-based) - klasican
//     minimalan skup za potpunu topolosku posmatranost.
//   - IEEE-3 (MATPOWER "case3bus_P6_6.m", extras/se folder - primjer iz udzbenika
//     "Computational Methods for Electric Power Systems", M. Crow, Problem 6.6),
//     PMU na busevima {1,2} (1-based) - mreza je trougao (svi cvorovi medjusobno
//     povezani), pa 2 PMU daju punu posmatranost sa redundansom.
#pragma once
#include "EstimatorCore.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <string>
#include <vector>

namespace se
{

enum class NetworkType { IEEE14, IEEE3 };

inline const char* systemFileName(NetworkType type)
{
    return (type == NetworkType::IEEE14) ? "system_ieee14.txt" : "system_ieee3.txt";
}

inline const char* networkDisplayName(NetworkType type)
{
    return (type == NetworkType::IEEE14) ? "IEEE-14" : "IEEE-3";
}

// ---- IEEE-14 topologija (0-based indeksi cvorova) ----
inline std::vector<Branch> ieee14Branches()
{
    //       from  to    R          X         B/2      tap
    return {
        {  0,  1,  0.01938,  0.05917,  0.0264,  1.0   },
        {  0,  4,  0.05403,  0.22304,  0.0246,  1.0   },
        {  1,  2,  0.04699,  0.19797,  0.0219,  1.0   },
        {  1,  3,  0.05811,  0.17632,  0.0170,  1.0   },
        {  1,  4,  0.05695,  0.17388,  0.0173,  1.0   },
        {  2,  3,  0.06701,  0.17103,  0.0064,  1.0   },
        {  3,  4,  0.01335,  0.04211,  0.0,     1.0   },
        {  3,  6,  0.0,      0.20912,  0.0,     0.978 },
        {  3,  8,  0.0,      0.55618,  0.0,     0.969 },
        {  4,  5,  0.0,      0.25202,  0.0,     0.932 },
        {  5, 10,  0.09498,  0.19890,  0.0,     1.0   },
        {  5, 11,  0.12291,  0.25581,  0.0,     1.0   },
        {  5, 12,  0.06615,  0.13027,  0.0,     1.0   },
        {  6,  7,  0.0,      0.17615,  0.0,     1.0   },
        {  6,  8,  0.0,      0.11001,  0.0,     1.0   },
        {  8,  9,  0.03181,  0.08450,  0.0,     1.0   },
        {  8, 13,  0.12711,  0.27038,  0.0,     1.0   },
        {  9, 10,  0.08205,  0.19207,  0.0,     1.0   },
        { 11, 12,  0.22092,  0.19988,  0.0,     1.0   },
        { 12, 13,  0.17093,  0.34802,  0.0,     1.0   },
    };
}

// ---- IEEE-3 topologija (MATPOWER case3bus_P6_6, 0-based indeksi cvorova) ----
// MATPOWER "b" kolona je UKUPNA punjenja susceptansa grane - nas Branch.b je B/2
// (dodaje se na oba kraja), pa se MATPOWER vrijednost dijeli sa 2. ratio=0 u MATPOWER-u
// znaci "nema transformatora" => tap=1.0.
inline std::vector<Branch> ieee3Branches()
{
    //       from  to    R      X     B/2 (=B_matpower/2)   tap
    return {
        { 0, 1,  0.01,  0.1,   0.050 / 2.0,  1.0 },
        { 0, 2,  0.05,  0.1,   0.025 / 2.0,  1.0 },
        { 1, 2,  0.05,  0.1,   0.025 / 2.0,  1.0 },
    };
}

// PMU cvorovi (0-based)
inline std::vector<int> pmuBuses(NetworkType type)
{
    if (type == NetworkType::IEEE14)
        return { 1, 5, 6, 8 };      // busevi 2,6,7,9 (1-based)
    return { 0, 1 };                 // IEEE-3: busevi 1,2 (1-based)
}

inline std::vector<Branch> networkBranches(NetworkType type)
{
    return (type == NetworkType::IEEE14) ? ieee14Branches() : ieee3Branches();
}

// Normalizovane [0,1]x[0,1] pozicije cvorova za crtanje sematskog dijagrama mreze
// (NetworkDiagramCanvas) - ocitane sa originalnih slika (res/IEEE-14.png, res/IEEE3.png)
// tako da raspored cvorova odgovara tim sematima.
// IEEE-3 (res/IEEE3.png): 3 sabirnice (lijevo/G1, dole/G2, desno/G3) povezane trouglom
// preko prekidaca - bus1=lijevo, bus2=dole, bus3=desno.
inline std::vector<std::pair<double, double>> networkNodePositions(NetworkType type)
{
    if (type == NetworkType::IEEE3)
    {
        return {
            { 0.14, 0.42 },   // bus1 (G1, lijevo)
            { 0.53, 0.75 },   // bus2 (G2, dole)
            { 0.85, 0.42 },   // bus3 (G3, desno)
        };
    }

    // IEEE-14 (res/IEEE-14.png), bus 1..14 => indeksi 0..13
    return {
        { 0.052, 0.401 }, { 0.148, 0.724 }, { 0.771, 0.842 }, { 0.697, 0.577 }, { 0.401, 0.509 },
        { 0.378, 0.401 }, { 0.823, 0.499 }, { 0.905, 0.401 }, { 0.757, 0.401 }, { 0.638, 0.294 },
        { 0.490, 0.294 }, { 0.141, 0.176 }, { 0.393, 0.108 }, { 0.593, 0.127 },
    };
}

// ---- Ucitavanje/snimanje kombinovanog fajla ----
// Format (whitespace-razdvojeno, "#" = komentar):
//   Branches:
//     busI busJ R X B tap
//   Measurements:
//     tip busI busJ z_re z_im sigma      (tip: V=fazor napona, I=fazor struje grane)

inline bool saveSystem(const std::string& path, const std::vector<Branch>& branches, const std::vector<PmuMeasurement>& meas)
{
    std::ofstream f(path);
    if (!f)
        return false;

    f << std::setprecision(15);
    f << "Branches:\n";
    for (const auto& b : branches)
        f << b.i << " " << b.j << " " << b.r << " " << b.x << " " << b.b << " " << b.tap << "\n";

    f << "Measurements:\n";
    for (const auto& m : meas)
        f << pmuMeasTypeToken(m.type) << " " << m.busI << " " << m.busJ << " "
          << m.z.real() << " " << m.z.imag() << " " << m.sigma << "\n";

    return true;
}

inline bool loadSystem(const std::string& path, std::vector<Branch>& branches, std::vector<PmuMeasurement>& meas)
{
    std::ifstream f(path);
    if (!f)
        return false;

    branches.clear();
    meas.clear();

    enum class Section { None, Branches, Measurements } section = Section::None;
    std::string line;
    while (std::getline(f, line))
    {
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos || line[start] == '#')
            continue;

        if (line.compare(start, 9, "Branches:") == 0)    { section = Section::Branches;    continue; }
        if (line.compare(start, 13, "Measurements:") == 0) { section = Section::Measurements; continue; }

        std::istringstream iss(line);
        if (section == Section::Branches)
        {
            Branch b;
            if (iss >> b.i >> b.j >> b.r >> b.x >> b.b >> b.tap)
                branches.push_back(b);
        }
        else if (section == Section::Measurements)
        {
            std::string typeToken;
            PmuMeasurement m;
            double zre = 0.0, zim = 0.0;
            if ((iss >> typeToken >> m.busI >> m.busJ >> zre >> zim >> m.sigma) && parsePmuMeasType(typeToken, m.type))
            {
                m.z = cmplx(zre, zim);
                meas.push_back(m);
            }
        }
    }
    return !branches.empty() && !meas.empty();
}

// "Pravo" stanje mreze (samo za generisanje sintetickih PMU mjerenja) - za IEEE-14 je
// to standardno (publikovano) rjesenje toka snage; za IEEE-3 (MATPOWER case3bus_P6_6)
// realisticne pretpostavljene vrijednosti (nazivni naponi busa 2,3 iz "gen" podataka,
// male ugaone razlike u smjeru toka snage).
inline std::vector<cmplx> networkTrueState(NetworkType type)
{
    auto deg2rad = [](double d) { return d * PI / 180.0; };

    if (type == NetworkType::IEEE14)
    {
        static const double vmagDeg[14][2] = {
            { 1.060,   0.00 }, { 1.045,  -4.98 }, { 1.010, -12.72 }, { 1.019, -10.33 },
            { 1.020,  -8.78 }, { 1.070, -14.22 }, { 1.062, -13.37 }, { 1.090, -13.36 },
            { 1.056, -14.94 }, { 1.051, -15.10 }, { 1.057, -14.79 }, { 1.055, -15.07 },
            { 1.050, -15.16 }, { 1.036, -16.04 },
        };
        std::vector<cmplx> Vtrue(14);
        for (int k = 0; k < 14; ++k)
            Vtrue[k] = std::polar(vmagDeg[k][0], deg2rad(vmagDeg[k][1]));
        return Vtrue;
    }

    // IEEE-3 (MATPOWER case3bus_P6_6): bus1=slack 1.00pu, bus2,3 su generatorski
    // busevi sa Vg=1.02pu (iz "gen" podataka)
    static const double vmagDeg3[3][2] = {
        { 1.00,  0.00 }, { 1.02, -3.50 }, { 1.02, -2.50 },
    };
    std::vector<cmplx> Vtrue(3);
    for (int k = 0; k < 3; ++k)
        Vtrue[k] = std::polar(vmagDeg3[k][0], deg2rad(vmagDeg3[k][1]));
    return Vtrue;
}

// Generise pocetni primjer (zadane mreze): grane + PMU fazorska mjerenja (napon na
// PMU cvorovima + struja na svakoj grani koja izlazi iz PMU cvora), izracunata iz
// "pravog" stanja mreze + realisticni PMU sum.
inline void generateExampleSystem(NetworkType type, std::vector<Branch>& branches, std::vector<PmuMeasurement>& meas)
{
    branches = networkBranches(type);
    std::vector<cmplx> Vtrue = networkTrueState(type);
    std::vector<int> pmus = pmuBuses(type);

    meas.clear();
    for (int p : pmus)
    {
        PmuMeasurement mv;
        mv.type = PmuMeasType::Voltage;
        mv.busI = p;
        mv.busJ = -1;
        mv.sigma = 0.0003;   // ~ PMU klasa tacnosti napona (red velicine 0.02-0.03%)
        meas.push_back(mv);
    }

    for (const auto& br : branches)
    {
        for (int p : pmus)
        {
            if (p != br.i && p != br.j)
                continue;
            int other = (p == br.i) ? br.j : br.i;

            PmuMeasurement mc;
            mc.type = PmuMeasType::Current;
            mc.busI = p;
            mc.busJ = other;
            mc.sigma = 0.0008;   // ~ PMU klasa tacnosti struje
            meas.push_back(mc);
        }
    }

    std::mt19937 rng(12345);   // fiksni seed radi ponovljivosti
    std::vector<cmplx> dummyJac;
    for (auto& m : meas)
    {
        cmplx trueVal = calcPmuMeasurement(m, branches, Vtrue, dummyJac);
        std::normal_distribution<double> noise(0.0, m.sigma);
        m.z = trueVal + cmplx(noise(rng), noise(rng));
    }
}

} // namespace se
