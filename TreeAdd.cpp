#include <ROOT/RDataFrame.hxx>
#include <TH3.h>
#include <TFile.h>
#include <vector>
#include <string>

TH3D* LoadEff(const std::vector<std::string>& files, const std::string& passName, const std::string& totName) {
	TFile f0(files[0].c_str(), "READ");
	auto hP = (TH3D*)f0.Get(passName.c_str())->Clone();
	auto hT = (TH3D*)f0.Get(totName.c_str())->Clone();
	hP->SetDirectory(nullptr);
	hT->SetDirectory(nullptr);

	for (size_t i = 1; i < files.size(); i++) {
		TFile f(files[i].c_str(), "READ");
		hP->Add((TH3D*)f.Get(passName.c_str()));
		hT->Add((TH3D*)f.Get(totName.c_str()));
	}

	hP->Divide(hT);
	return hP;
}

void TreeAddEff() {
    ROOT::RDataFrame df("Tuple_SpruceSLB_B0ToDpMuNu_DpToKPiPi/DecayTree", "MC_2.root"); // change the MC name and the directory etc

	auto hK = LoadEff({"K1.root", "K2.root"}, // name of the ROOT files containing the efficiencies for the specific particle
		"passing_DLLK>4&PROBNN_K>0.3", "total"); // names of passing and total
	
	auto hPi = LoadEff({"PI1.root", "PI2.root"}, "passing_DLLK<2&PROBNN_PI>0.3", "total");

	auto hMu = LoadEff({"M1.root", "M2.root"}, "passing_DLLmu>0&IsMuon==1&PROBNN_MU>0.3","total");

	auto effK = [hK](float P, float ETA, int nLongTracks) {
		return hK->GetBinContent(hK->FindFixBin(P, ETA, nLongTracks));
	};

	auto effPi = [hPi](float P, float ETA, int nLongTracks) {
		return hPi->GetBinContent(hPi->FindFixBin(P, ETA, nLongTracks));
	};

	auto effMu = [hMu](float P, float ETA, int nLongTracks) {
		return hMu->GetBinContent(hMu->FindFixBin(P, ETA, nLongTracks));
	};

    auto df2 = df
                .Define("eff_K", effK, {"Dp_Kp_P",   "Dp_Kp_ETA",   "nLongTracks"})
                .Define("eff_pi1",effPi, {"Dp_pim1_P", "Dp_pim1_ETA", "nLongTracks"})
                .Define("eff_pi2",effPi, {"Dp_pim2_P", "Dp_pim2_ETA", "nLongTracks"})
                .Define("eff_mu", effMu, {"Bz_mup_P",  "Bz_mup_ETA",  "nLongTracks"})
                .Define("eff_Dp",[](double ek, double ep1, double ep2){ 
					return ek * ep1 * ep2; 
				}, {"eff_K", "eff_pi1", "eff_pi2"})
                .Define("eff_Bz",[](double ek, double ep1, double ep2, double emu){ 
					return ek * ep1 * ep2 * emu; 
				}, {"eff_K", "eff_pi1", "eff_pi2", "eff_mu"});    

	df2.Snapshot("DecayTree", "MC_2_eff.root"); // name of new root file
}
