// EstimatorCore.h : PMU (fazorska) estimacija stanja u kompleksnim koordinatama,
// zasnovana na Wirtinger kalkulusu. Broj cvorova NIJE fiksan na prevodenju (podrzava
// vise razlicitih mreza - IEEE-14, IEEE-3 iz MATPOWER-a, itd.) - odredjuje se iz
// ucitanih/generisanih podataka grana u trenutku pokretanja.
//
// Za razliku od "legacy" SCADA mjerenja (P,Q,|V| - REALNE funkcije od (V,V*), gdje
// je potreban Wirtinger izvod po V* jer h nije holomorfno u V), fazorska (PMU) mjerenja
// napona i struje su LINEARNE (holomorfne) funkcije napona V - dh/dV* = 0 (vidi
// Dzafic, Jabr, Hrnjic, "Hybrid State Estimation in Complex Variables", IEEE TPWRS 2018,
// Appendix, jednacine 40-41 i 48-49 za slucaj bez SCADA mjerenja).
//
// Zato se ovdje koristi OBICAN (ne konjugovani) kompleksni Jakobijan A=dh/dV, a
// Gauss-Newton korak je standardna kompleksna WLS: (A^H W A) dV = A^H W r - rjesava se
// preko dense::CmplxMatrix (Matrix.dll), isto kao i ranija (SCADA) verzija, samo je
// sada Jakobijan trivijalan (konstantni koeficijenti grane) jer je mjerna funkcija
// linearna u V. Posljedica: za PMU-only mjerenja algoritam konvergira u JEDNOJ iteraciji
// (tacno kao sto je i navedeno u referentnom radu za PMU-only slucaj).
#pragma once
#include <compiler/MinMaxFix.h>   // windows.h (preko gui headera) definise min/max makroe
#include <dense/Matrix.h>
#include <td/Types.h>

#include <vector>
#include <complex>
#include <cmath>
#include <string>
#include <algorithm>

namespace se
{

using cmplx = td::cmplx;

constexpr double PI = 3.14159265358979323846;

// Napomena: svi naponi su nepoznati (nema fiksnog "slack" cvora u vektoru stanja) -
// bilo koje PMU naponsko mjerenje daje apsolutnu ugaonu (i amplitudsku) referencu,
// pa ovdje (za razliku od SCADA verzije) nije potrebno posebno fiksirati slack cvor.

struct Branch
{
    int i, j;          // 0-based indeksi cvorova (from, to)
    double r, x;        // serijska otpornost/reaktansa [pu]
    double b;            // shant susceptansa B/2 (dodaje se na oba kraja) [pu]
    double tap;           // transformatorski odnos (1.0 = obican vod bez transformatora)
};

// Y-clan lokalnog doprinosa struji cvora
struct YTerm
{
    int bus;
    cmplx y;
};

enum class PmuMeasType { Voltage, Current };

struct PmuMeasurement
{
    PmuMeasType type;
    int busI;           // cvor na kojem je PMU (za struju: cvor "iz kojeg gleda" u granu)
    int busJ;             // "prema" cvoru za strujno mjerenje, inace -1
    cmplx z = cmplx(0.0, 0.0);    // izmjerena (kompleksna) vrijednost
    double sigma;                  // std. devijacija (ista za realni i imaginarni dio)
};

struct EstimationResult
{
    std::vector<cmplx> V;      // V[0..nBus-1]
    int iterations = 0;
    double maxDx = 0.0;
    bool converged = false;
    bool singular = false;
};

// Broj cvorova mreze - izvodi se iz podataka grana (najveci indeks + 1), umjesto
// da bude fiksna konstanta, tako da se ista implementacija koristi za bilo koju
// ucitanu/generisanu mrezu (IEEE-14, IEEE-3, ...).
inline int busCount(const std::vector<Branch>& branches)
{
    int maxIdx = -1;
    for (const auto& br : branches)
        maxIdx = std::max({maxIdx, br.i, br.j});
    return maxIdx + 1;
}

// -------------------- Mreza (IEEE-14, sa transformatorskim odnosom i B/2 shantom) --------------------
//
// Standardni model transformatorske grane (isti kao u referentnom MATLAB "ybusppg.m"):
//   Y[i][i] += y/a^2 + j*B/2
//   Y[j][j] += y      + j*B/2
//   Y[i][j] = Y[j][i] = -y/a
// gdje je y = 1/(R+jX), a = tap (odnos), i = "from" cvor grane.

inline std::vector<std::vector<cmplx>> buildYbus(int nBus, const std::vector<Branch>& branches)
{
    std::vector<std::vector<cmplx>> Y(nBus, std::vector<cmplx>(nBus, cmplx(0.0, 0.0)));
    for (const auto& br : branches)
    {
        cmplx y = 1.0 / cmplx(br.r, br.x);
        cmplx ysh(0.0, br.b);
        double a = br.tap;

        Y[br.i][br.i] += y / (a * a) + ysh;
        Y[br.j][br.j] += y + ysh;
        Y[br.i][br.j] -= y / a;
        Y[br.j][br.i] -= y / a;
    }
    return Y;
}

// Lokalni ("granski") koeficijenti struje mjerene NA cvoru p (p mora biti br.i ili br.j),
// gledano iz p prema drugom kraju grane - ista logika kao Ybus konstrukcija, ali samo
// za JEDNU granu (bez sumiranja preko svih grana povezanih na cvor).
inline std::vector<YTerm> branchCurrentTerms(const Branch& br, int p)
{
    cmplx y = 1.0 / cmplx(br.r, br.x);
    cmplx ysh(0.0, br.b);
    double a = br.tap;

    if (p == br.i)
    {
        cmplx y_pp = y / (a * a) + ysh;
        cmplx y_pq = -y / a;
        return { {br.i, y_pp}, {br.j, y_pq} };
    }
    else
    {
        cmplx y_pp = y + ysh;
        cmplx y_pq = -y / a;
        return { {br.j, y_pp}, {br.i, y_pq} };
    }
}

// -------------------- PMU mjerna funkcija h(V) i (holomorfni) Jakobijan dh/dV --------------------

inline cmplx calcPmuMeasurement(const PmuMeasurement& m, const std::vector<Branch>& branches,
                                 const std::vector<cmplx>& V, std::vector<cmplx>& jacRow)
{
    jacRow.assign(V.size(), cmplx(0.0, 0.0));

    if (m.type == PmuMeasType::Voltage)
    {
        jacRow[m.busI] = cmplx(1.0, 0.0);   // dh/dV_i = 1 (identitet - fazorsko mjerenje napona)
        return V[m.busI];
    }

    // PmuMeasType::Current - pronadji granu (m.busI, m.busJ) i primjeni granske koeficijente
    for (const auto& br : branches)
    {
        if ((br.i == m.busI && br.j == m.busJ) || (br.j == m.busI && br.i == m.busJ))
        {
            auto terms = branchCurrentTerms(br, m.busI);
            cmplx I(0.0, 0.0);
            for (const auto& t : terms)
            {
                I += t.y * V[t.bus];
                jacRow[t.bus] += t.y;   // dh/dV = koeficijent grane (konstanta - h je linearno u V)
            }
            return I;
        }
    }
    return cmplx(0.0, 0.0);   // grana nije pronadjena (ne treba se desiti za validne podatke)
}

// Kompleksni Gauss-Newton za HOLOMORFNA (linearna) mjerenja: h analiticko u V (dh/dV*=0),
// pa vazi standardna kompleksna WLS: (A^H W A) dV = A^H W r,  A_k = dh_k/dV.
// Matrix::fromMult() provjerava A.getNoOfCols()==B.getNoOfRows() na sirovim (fizickim)
// dimenzijama, pa se Hermitsko (konjugovano) transponovanje mora uraditi eksplicitno.
inline EstimationResult estimateStatePMU(const std::vector<Branch>& branches, const std::vector<PmuMeasurement>& meas)
{
    const int nBus = busCount(branches);

    EstimationResult result;
    result.V.assign(nBus, cmplx(1.0, 0.0));   // flat start

    const int maxIter = 10;
    const double tol = 1e-8;

    std::vector<cmplx>& V = result.V;
    int iter = 0;
    double maxDx = 0.0;

    for (; iter < maxIter; ++iter)
    {
        dense::CmplxMatrix A(static_cast<td::UINT4>(meas.size()), nBus);
        dense::CmplxMatrix rw(static_cast<td::UINT4>(meas.size()), 1);
        auto aIO = A.getManipulator();
        auto rIO = rw.getColumnManipulator();

        std::vector<cmplx> jacRow;
        for (size_t k = 0; k < meas.size(); ++k)
        {
            const td::UINT4 row = static_cast<td::UINT4>(k);
            const auto& m = meas[k];
            cmplx hVal = calcPmuMeasurement(m, branches, V, jacRow);
            cmplx res = m.z - hVal;
            double w = 1.0 / m.sigma;

            for (int c = 0; c < nBus; ++c)
                aIO(row, c) = jacRow[c] * w;
            rIO(row) = res * w;
        }

        dense::CmplxMatrix Ah = A.transpose();

        dense::CmplxMatrix G(nBus, nBus);
        dense::CmplxMatrix rhs(nBus, 1);
        G.fromMult(Ah, A, "N", "N");
        rhs.fromMult(Ah, rw, "N", "N");

        if (!G.solve(rhs))
        {
            result.singular = true;
            result.iterations = iter;
            return result;
        }

        auto dV = rhs.getColumnManipulator();
        maxDx = 0.0;
        for (int c = 0; c < nBus; ++c)
        {
            V[c] += dV(c);
            maxDx = std::max(maxDx, std::abs(dV(c)));
        }

        if (maxDx < tol)
        {
            ++iter;
            break;
        }
    }

    result.iterations = iter;
    result.maxDx = maxDx;
    result.converged = (maxDx < tol);
    return result;
}

inline const char* pmuMeasTypeToken(PmuMeasType t)
{
    return (t == PmuMeasType::Voltage) ? "V" : "I";
}

inline bool parsePmuMeasType(const std::string& token, PmuMeasType& type)
{
    if (token == "V") { type = PmuMeasType::Voltage; return true; }
    if (token == "I") { type = PmuMeasType::Current; return true; }
    return false;
}

} // namespace se
