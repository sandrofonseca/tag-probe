 /*
!--------------------------------
!proposito: ajustar funções sobre histogramas no auxilio do tag and probe
!--------------------------------	
!autor: Allan Jales
!--------------------------------
*/

#include "TROOT.h"
#include "TMath.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TFitResult.h"
#include "TLegend.h"
#include "TLatex.h"

#include <TF1NormSum.h>

#include <iostream>

using namespace std;



//-------------------------------------
// Fit functions for Invariant Mass
//-------------------------------------

class FitFunctions
{
public:
	class Primary
	{
	public:
		//Gaussian function
		static Double_t Gaus(Double_t *x, Double_t *par)
		{
			//par[0] = height
			//par[1] = position
			//par[2] = sigma
			Double_t gaus = par[0]*TMath::Gaus(x[0],par[1],par[2],1);
			return gaus;
		}

		//Polynomial function
		static Double_t Pol1(Double_t *x, Double_t *par)
		{
			//par[0] = b
			//par[1] = a
			Double_t pol = par[0] + par[1]*x[0];
			return pol;
		}

		//Exponential function
		static Double_t Exp(Double_t *x, Double_t *par)
		{
			//par[0] = height
			//par[1] = width
			Double_t exp = par[0] * TMath::Exp(par[1]*x[0]);
			return exp;
		}

		//crystall ball function
		static Double_t CrystalBall(Double_t *x,Double_t *par)
		{
			//par[0] = alpha
			//par[1] = n
			//par[2] = mean
			//par[3] = sigma
			//par[4] = Yield
			Double_t t = (x[0]-par[2])/par[3];
			if (par[0] < 0) t = -t;
			Double_t absAlpha = fabs((Double_t)par[0]);
			if (t >= -absAlpha)
			{
				return par[4]*exp(-0.5*t*t);
			}
			else
			{
				Double_t a =  TMath::Power(par[1]/absAlpha,par[1])*exp(-0.5*absAlpha*absAlpha);
				Double_t b = par[1]/absAlpha - absAlpha;
				return par[4]*(a/TMath::Power(b - t, par[1]));
			}
		}
	};
	class Merged
	{
	public:
		//Fit function for signal for Invariant Mass Probe
		static Double_t Signal_InvariantMassAll(Double_t *x, Double_t *par) {
			return FitFunctions::Primary::Gaus(x,par) + FitFunctions::Primary::CrystalBall(x, &par[3]);
		}

		//Fit function for background for Invariant Mass Probe
		static Double_t Background_InvariantMassAll(Double_t *x, Double_t *par) {
			return FitFunctions::Primary::Exp(x,par) + FitFunctions::Primary::Exp(x, &par[2]);
		}

		//Fit function for signal & background for Invariant Mass Probe
		static Double_t FFit_InvariantMassAll(Double_t *x, Double_t *par) {
			return FitFunctions::Merged::Signal_InvariantMassAll(x,par) + FitFunctions::Merged::Background_InvariantMassAll(x, &par[8]);
		}
	};
};

//-------------------------------------
// Main functions
//-------------------------------------

class InvariantMassClass{
public:
	Int_t 			nBins;
	int 			decimals = 4;
	Double_t 		xMin;
	Double_t		xMax;

	TH1D*			hMass;
	TF1*			fitFunction;
	TF1*			fitFunctionSig;
	TF1*			fitFunctionBack;
	TFitResultPtr 	fitResult;

	const char* const fittingParName[12] = {
			"Gaus(Sg) Height  ",
			"Gaus(Sg) Position",
			"Gaus(Sg) Sigma   ",

			"CB  (Sg) Alpha   ",
			"CB  (Sg) N       ",
			"CB  (Sg) Mean    ",
			"CB  (Sg) Sigma   ",
			"CB  (Sg) Yield   ",

			"Exp1(Bg) Lambda  ",
			"Exp1(Bg) a       ",
			"Exp2(Bg) Lambda  ",
			"Exp2(Bg) a       "
		};

	Double_t resultParameters[12];

	int 	count_signalRegion = 0;
	int 	count_sidebandRegion = 0;
	double 	S  = 0;
	double 	dS = 0;

	void sidebandCalculus()
	{
		S = count_signalRegion - count_sidebandRegion;
		dS = sqrt(count_signalRegion + count_sidebandRegion);
	}

	void defineNumbers(Int_t nBins, Double_t xMin, Double_t xMax, int decimals = 4)
	{
		this->nBins 	= nBins;
		this->xMin 		= xMin;
		this->xMax 		= xMax;
		this->decimals 	= decimals;
	}

	void createMassHistogram()
	{
		string yAxisTitleForm 	= "Events / (%1." + to_string(decimals) + "f GeV/c^{2})";

		//Create histogram
		hMass = new TH1D("Muon_InvariantMass", "Invariant Mass (All);Mass (GeV/c^{2});Events", nBins, xMin, xMax);
		hMass->GetYaxis()->SetTitle(Form(yAxisTitleForm.data(), hMass->GetBinWidth(0)));
	}

	/*
	void fit()
	{
		TF1 *fs = new TF1("fs",Signal_InvariantMassAll,xMin,xMax,8);
		fs->SetParName(0,	"Gaus(Sg) Height");
		fs->SetParName(1,	"Gaus(Sg) Position");
		fs->SetParName(2,	"Gaus(Sg) Sigma");
		fs->SetParName(3,	"CB(Sg) Alpha");
		fs->SetParName(4,	"CB(Sg) N");
		fs->SetParName(5,	"CB(Sg) Mean");
		fs->SetParName(6,	"CB(Sg) Sigma");
		fs->SetParName(7,	"CB(Sg) Yield");
		
		//Values Signal
		fs->SetParameter(0,	86.2327);
		fs->SetParameter(1,	3.09508);
		fs->SetParameter(2,	0.0389252);

		fs->SetParameter(3,	1.83218);
		fs->SetParameter(4,	0.821031);
		fs->SetParameter(5,	3.09279);
		fs->SetParameter(6,	0.0227687);
		fs->SetParameter(7,	1827.62);

		TF1 *fb = new TF1("fb",Background_InvariantMassAll,xMin,xMax,4);
		fb->SetParName(0,	"Exp1(Bg) Lambda");
		fb->SetParName(1,	"Exp1(Bg) a");
		fb->SetParName(2,	"Exp2(Bg) Lambda");
		fb->SetParName(3,	"Exp2(Bg) a");

		//Values Background
		fb->SetParameter(0,	-0.0102751);
		fb->SetParameter(1,	2.17821);
		fb->SetParameter(2, 23.9689);
		fb->SetParameter(3, 0.367475);

		//Sum functions
		TF1NormSum *fnorm = new TF1NormSum(fs,fb);
		TF1   * f = new TF1("f", *fnorm, xMin, xMax, fnorm->GetNpar());
		f->SetParameters(fnorm->GetParameters().data());
		f->SetParName(1,"NBackground");
		f->SetParName(0,"NSignal");
		for (int i = 2; i < f->GetNpar(); ++i)
			f->SetParName(i, fnorm->GetParName(i));
		f->SetNpx(1000);	//Resolution of fit function

		//Fit Color
		f->SetLineColor(kBlue);

		//Fit the function (Remove Q to standard show)
		TFitResultPtr fitr = hMassAll->Fit(f,"RNS","",xMin,xMax);
		//fitr->Print();
		
		//Signal Fitting
		fs->SetNpx(1000);				//Resolution of background fit function
		fs->SetLineColor(kMagenta); 	//Fit Color
		fs->SetLineStyle(kSolid);		//Fit Style

		//Background Fitting
		fb->SetNpx(1000);				//Resolution of background fit function
		fb->SetLineColor(kBlue); 		//Fit Color
		fb->SetLineStyle(kDashed);		//Fit Style
	}
	*/

	void fit()
	{
		//Create fit Function
		fitFunction = new TF1("FitFunction", FitFunctions::Merged::FFit_InvariantMassAll, xMin, xMax, 12);

		//Rename parameters and set value
		for (int i = 0; i < (sizeof(fittingParName)/sizeof(*fittingParName)); i++)
		{
			fitFunction->SetParName(i, fittingParName[i]);
		}

		//Resolution of fit function
		fitFunction->SetNpx(1000);
		
		//Values Signal
		fitFunction->SetParameter(0,	340.2);
		fitFunction->SetParameter(1,	3.09);
		fitFunction->SetParameter(2,	0.037);

		fitFunction->SetParameter(3,	1.824);
		fitFunction->SetParameter(4,	1.034);
		fitFunction->SetParameter(5,	3.093);
		fitFunction->SetParameter(6,	0.022);
		fitFunction->SetParameter(7,	8322.27);

		//Values Background
		fitFunction->SetParameter(8,	-0.217);
		fitFunction->SetParameter(9,	1.915);
		fitFunction->SetParameter(10, 263.185);
		fitFunction->SetParameter(11,	0.061);

		//Fit Color
		fitFunction->SetLineColor(kBlue);

		//Fit the function
		fitResult = hMass->Fit(fitFunction, "RNS", "", xMin, xMax);

		//Get parameters from fit function and put it in par variable
		fitFunction->GetParameters(resultParameters);
		
		//Signal Fitting
		fitFunctionSig = new TF1("FitFunction_Signal", FitFunctions::Merged::Signal_InvariantMassAll, xMin, xMax, 8);
		fitFunctionSig->SetNpx(1000);							//Resolution of background fit function
		fitFunctionSig->SetParameters(resultParameters);		//Get only background part
		fitFunctionSig->SetLineColor(kMagenta); 				//Fit Color
		fitFunctionSig->SetLineStyle(kSolid);					//Fit Style

		//Background Fitting
		fitFunctionBack = new TF1("FitFunction_Background", FitFunctions::Merged::Background_InvariantMassAll, xMin, xMax, 4);
		fitFunctionBack->SetNpx(1000);							//Resolution of background fit function
		fitFunctionBack->SetParameters(&resultParameters[8]);	//Get only background part
		fitFunctionBack->SetLineColor(kBlue); 					//Fit Color
		fitFunctionBack->SetLineStyle(kDashed);					//Fit Style
	}

	TCanvas *createCanvas(bool shouldWrite = false, bool shouldSave = false)
	{
		const char* saveAs = "../InvariantMassAll.png";

		//Create canvas
		TCanvas *c1 = new TCanvas("InvariantMass_All","Invariant Mass", 600, 600);

		//Set margin for canvas
		c1->SetTopMargin(0.07);
		c1->SetRightMargin(0.05);
		c1->SetBottomMargin(0.11);
		c1->SetLeftMargin(0.15);

		//Set title size for axis and other stuffs for histogram style
		hMass->GetYaxis()->SetTitleSize(0.04);
		hMass->GetXaxis()->SetTitleSize(0.05);
		hMass->GetXaxis()->CenterTitle(true);
		//hMassAll->SetBit(TH1::kNoTitle);		//Not show histogram title
		hMass->SetMarkerStyle(20);				//Set markers style
		hMass->SetMarkerColor(kBlack);			//Set markers colors
		hMass->SetLineColor(kBlack);			//Set lines colors (for errorbars)

		//Get min and max from histogram
		double xMin  = hMass->GetXaxis()->GetXmin();
		double xMax  = hMass->GetXaxis()->GetXmax();
		double scale = 1/hMass->GetBinWidth(0);		//How many bins there is in each x axis unit / For integral 

		//Calls fit function
		fit();

		//Draws histogram & fit function
		hMass->Draw("ep");
		//fitFunctionSig->Draw("same");
		fitFunctionBack->Draw("same");
		fitFunction->Draw("same");

		//Draws information
		TLatex *tx = new TLatex();
		tx->SetTextSize(0.04);
		tx->SetTextAlign(12);
		tx->SetTextFont(42);
		tx->SetNDC(kTRUE);

		//Show chi-squared test
		tx->DrawLatex(0.61,0.60,Form("#chi^{2}/ndf = %.3g",fitResult->Chi2()/fitResult->Ndf()));

		/*
		//Show number of particles
		tx->DrawLatex(0.61,0.48,Form("%.0f #pm %.0f J/#psi", S, dS));
		tx->DrawLatex(0.64,0.43,"(candidates)");
		*/

		//Add legend
		TLegend *l = new TLegend(0.65,0.77,0.92,0.90);
		l->SetTextSize(0.04);
		l->AddEntry(hMass,				"J/#psi"	,"lp");
		l->AddEntry(fitFunction,		"Fitting"	,"l");
		//l->AddEntry(fitFunctionSig,	"Signal"	,"l");
		l->AddEntry(fitFunctionBack,	"Background","l");
		l->Draw();

		//Not show frame with mean, std dev
		gStyle->SetOptStat(0);

		//Get Y range of draw
		gPad->Update();
		Double_t Ymax = gPad->GetFrame()->GetY2();
		
		/*
		//Draw boxes
		gStyle->SetCanvasPreferGL(kTRUE);
		TBox *side1 = new TBox(2.9, 0., 3., Ymax);
		side1->SetFillColorAlpha(kRed, 0.35);
		side1->Draw();
		TBox *signal = new TBox(3., 0., 3.2, Ymax);
		signal->SetFillColorAlpha(kGreen, 0.35);
		signal->Draw();
		TBox *side2 = new TBox(3.2, 0.,3.3, Ymax);
		side2->SetFillColorAlpha(kRed, 0.35);
		side2->Draw();
		*/
		
		//Show chi-squared test (on prompt)
		cout << endl;
		cout << "Fitting overview" << endl;
		cout << "Chi2/ndf = " << fitFunction->GetChisquare()/fitFunction->GetNDF() << endl;
		printf( "#Signal  = %.0f +- %.0f\n", S, dS);

		cout << endl;
		cout << "HistIntegral = " << hMass->Integral(0, hMass->GetNbinsX()) << endl;

		//Show integrals
		cout << endl;
		cout << "Candidates by integration" << endl;
		cout << "#Total      = " << fitFunction    ->Integral(xMin, xMax) * scale /*<< " +- " << fitFunction    ->IntegralError(xMin, xMax) * scale*/ << endl;
		cout << "#Background = " << fitFunctionBack->Integral(xMin, xMax) * scale /*<< " +- " << fitFunctionBack->IntegralError(xMin, xMax) * scale*/ << endl;
		cout << "#Signal     = " << fitFunctionSig ->Integral(xMin, xMax) * scale /*<< " +- " << fitFunctionSig ->IntegralError(xMin, xMax) * scale*/ << endl;

		//Writes in file
		if (shouldWrite == true)
		{
			//Here code crashes when it runs for the 2nd time
			c1->Write();
		}

		//If should save
		if (shouldSave == true)
		{
			//Saves as image
			c1->SaveAs(saveAs);
		}

		//return
		return c1;

	}
};

class Histograms{
public:
	const char *quantityName;
	const char *extendedQuantityName;
	const char *quantityUnit;
	const char *xAxisName;
	const char *tagOrProbe;
	const char *particle = "Muon";

	Int_t 		nBins;
	int 		decimals = 3;
	Double_t 	xMin;
	Double_t	xMax;

	TH1D *hSigBack;
	TH1D *hSig;
	TH1D *hBack;

	void defineTexts(const char *quantityName, const char *xAxisName, const char *quantityUnit,const char *extendedQuantityName, const char *tagOrProbe, const char *particle = "Muon")
	{
		this->quantityName 			= quantityName;
		this->extendedQuantityName 	= extendedQuantityName;
		this->xAxisName 			= xAxisName;
		this->quantityUnit			= quantityUnit;
		this->tagOrProbe 			= tagOrProbe;
		this->particle 				= particle;
	}

	void defineNumbers(Int_t nBins, Double_t xMin, Double_t xMax, int decimals = 3)
	{
		this->nBins 	= nBins;
		this->xMin 		= xMin;
		this->xMax 		= xMax;
		this->decimals 	= decimals;
	}

	void createSigBackHistogram()
	{
		//Define parameters
		string hName 		= tagOrProbe + string(particle) + "_" + string(quantityName) + "SigBack";
		string hTitle 		= string(extendedQuantityName) + " (" + string(tagOrProbe) + ")";
		string xAxisTitle 	= xAxisName;
		string yAxisTitleForm;
		if (strcmp(quantityUnit, "") == 0)
		{
			yAxisTitleForm 	= "Events / (%1." + to_string(decimals) + "f)";
		}
		else
		{
			xAxisTitle += " (" + string(quantityUnit) + ")";
			yAxisTitleForm 	= "Events / (%1." + to_string(decimals) + "f " + string(quantityUnit) + ")";
		}

		//Create histogram
		hSigBack = new TH1D(hName.data(), hTitle.data(), nBins, xMin, xMax);
		hSigBack->GetYaxis()->SetTitle(Form(yAxisTitleForm.data(), hSigBack->GetBinWidth(0)));
		hSigBack->GetXaxis()->SetTitle(xAxisTitle.data());
	}

	void createBackHistogram()
	{
		//Define parameters
		string hName = tagOrProbe + string(particle) + "_" + string(quantityName) + "Back";
	
		//Create histogram
		hBack = (TH1D*) hSigBack->Clone("hBack");
		hBack->SetName(hName.data());
	}

	void createSigHistogram()
	{
		//Define parameters
		string hName = tagOrProbe + string(particle) + "_" + string(quantityName) + "Sig";
	
		//Create histogram
		hSig = (TH1D*) hSigBack->Clone("hSig");
		hSig->SetName(hName.data());
		hSig->Add(hBack,-1);
	}

	TCanvas *createDividedCanvas(bool shouldWrite = false, bool shouldSave = true)
	{
		string canvasName 	= tagOrProbe + string(particle) + "_" + string(quantityName);
		string titleLeft 	= string(extendedQuantityName) + " (" + string(tagOrProbe) + ")";
		string titleRight 	= string(extendedQuantityName) + " of Signal (" + string(tagOrProbe) + ")";
		string saveAs 		= "../" + string(quantityName) + string(tagOrProbe) + ".png";

		//Create canvas and divide it
		TCanvas *c1 = new TCanvas(canvasName.data(), titleLeft.data(), 1200, 600);
		c1->Divide(2,1);

		//Select canvas part and set margin
		c1->cd(1);
		c1->cd(1)->SetMargin(0.14, 0.03, 0.11, 0.07);

		//Draws Main histogram
		hSigBack->SetMinimum(0);
		hSigBack->SetLineWidth(2);		//Line Width
		hSigBack->Draw();

		//Draws Background histogram
		hBack->SetLineColor(kBlue); 	//Line Color
		hBack->SetLineStyle(kDashed);	//Line Style
		hBack->SetLineWidth(2);			//Line Width
		hBack->Draw("same");

		//Draws Signal histogram
		hSig->SetLineColor(kMagenta); 	//Line Color
		hSig->SetLineWidth(2);			//Line Width
		hSig->Draw("same");

		//Get Y range of draw
		gPad->Update();
		Double_t Ymax = gPad->GetFrame()->GetY2();

		/*
		//Set fill color
		hSig->SetFillColor(kMagenta);
		hSigBack->SetFillColor(kBlue);
		hBack->SetFillColor(kYellow);
		*/

		//Add legend
		TLegend *l1_1 = new TLegend(0.65,0.75,0.92,0.90);
		l1_1->SetTextSize(0.04);
		l1_1->AddEntry(hSigBack,	"All",			"lp");
		l1_1->AddEntry(hSig,		"Signal",		"l");
		l1_1->AddEntry(hBack,		"Background",	"l");
		l1_1->Draw();
		
		//Draws text information
		TLatex *tx1_1 = new TLatex();
		tx1_1->SetTextSize(0.04);
		tx1_1->SetTextFont(42);
		tx1_1->SetNDC(kTRUE);
		if (strcmp(quantityName, "Pt") == 0) 
		{
			tx1_1->SetTextAlign(12);	//Align left, center
			tx1_1->DrawLatex(0.48,0.50,Form("%g entries (total)",		hSigBack->GetEntries()));
			tx1_1->DrawLatex(0.48,0.45,Form("%g entries (signal)",		hSig->GetEntries()));
			tx1_1->DrawLatex(0.48,0.40,Form("%g entries (background)",	hBack->GetEntries()));
		}
		else
		{
			tx1_1->SetTextAlign(22);	//Align center, center
			tx1_1->DrawLatex(0.55,0.50,Form("%g entries (total)",		hSigBack->GetEntries()));
			tx1_1->DrawLatex(0.55,0.45,Form("%g entries (signal)",		hSig->GetEntries()));
			tx1_1->DrawLatex(0.55,0.40,Form("%g entries (background)",	hBack->GetEntries()));
		}
		

		//Select canvas part and set margin
		c1->cd(2);
		c1->cd(2)->SetMargin(0.14, 0.03, 0.11, 0.07);

		//Same range as comparision and draws
		hSig->SetMinimum(0);
   		//hSig->SetMaximum(Ymax);
		hSig->SetTitle(titleRight.data());
		hSig->Draw("same");

		//Add legend
		TLegend *l1_2 = new TLegend(0.65,0.85,0.92,0.90);
		l1_2->SetTextSize(0.04);
		l1_2->AddEntry(hSig, "Signal","l");
		l1_2->Draw();

		//Draws text information
		TLatex *tx1_2 = new TLatex();
		tx1_2->SetTextSize(0.04);
		tx1_2->SetTextFont(42);
		tx1_2->SetNDC(kTRUE);
		if (strcmp(quantityName, "Pt") == 0)
		{
			tx1_2->SetTextAlign(12);	//Align left, center
			tx1_2->DrawLatex(0.48,0.50,Form("%g entries (signal)", hSig->GetEntries()));
		}
		else
		{
			tx1_2->SetTextAlign(22);	//Align center, center
			tx1_2->DrawLatex(0.55,0.5,Form("%g entries (signal)", hSig->GetEntries()));
		}

		//Not show frame with mean, std dev
		gStyle->SetOptStat(0);

		//Writes in file
		if (shouldWrite == true)
		{
			c1->Write();
		}

		//If should save
		if (shouldSave == true)
		{
			//Saves as image
			c1->SaveAs(saveAs.data());
		} 

		//return
		return c1;
	}
};

//Bunch of 2 histogram class for Tag and Probe
class TagAndProbe{
public:
	const char *tagOrProbe;

	Histograms Pt;
	Histograms Eta;
	Histograms Phi;

	void define(const char *tagOrProbe)
	{
		this->tagOrProbe = tagOrProbe;
	}

	void defineHistogramsTexts()
	{
		this->Pt .defineTexts("Pt",  "P_{t}",	"GeV/c", 	"Transversal Momentum", tagOrProbe);
		this->Eta.defineTexts("Eta", "#eta", 	"", 		"Pseudorapidity", 		tagOrProbe);
		this->Phi.defineTexts("Phi", "#phi", 	"", 		"Azimuthal Angle", 		tagOrProbe);
	}

	void defineHistogramsNumbers()
	{
		this->Pt .defineNumbers(100,	 0., 	100., 1);
		this->Eta.defineNumbers(200, 	-2.5, 	2.5);
		this->Phi.defineNumbers(79, 	-3.15, 	3.15);
	}

	void fillSigBackHistograms(Double_t PtValue, Double_t EtaValue, Double_t PhiValue)
	{
		this->Pt .hSigBack->Fill(PtValue);
		this->Eta.hSigBack->Fill(EtaValue);
		this->Phi.hSigBack->Fill(PhiValue);
	}

	void fillSigHistograms(Double_t PtValue, Double_t EtaValue, Double_t PhiValue)
	{
		this->Pt .hSig->Fill(PtValue);
		this->Eta.hSig->Fill(EtaValue);
		this->Phi.hSig->Fill(PhiValue);
	}

	void fillBackHistograms(Double_t PtValue, Double_t EtaValue, Double_t PhiValue)
	{
		this->Pt .hBack->Fill(PtValue);
		this->Eta.hBack->Fill(EtaValue);
		this->Phi.hBack->Fill(PhiValue);
	}

	void createSigBackHistograms()
	{
		this->Pt .createSigBackHistogram();
		this->Eta.createSigBackHistogram();
		this->Phi.createSigBackHistogram();
	}

	void createBackHistograms()
	{
		this->Pt .createBackHistogram();
		this->Eta.createBackHistogram();
		this->Phi.createBackHistogram();
	}

	void createSigHistograms()
	{
		this->Pt .createSigHistogram();
		this->Eta.createSigHistogram();
		this->Phi.createSigHistogram();
	}

	void createDividedCanvas(bool shouldWrite = false, bool shouldSave = false)
	{
		this->Pt .createDividedCanvas(shouldWrite, shouldSave);
		this->Eta.createDividedCanvas(shouldWrite, shouldSave);
		this->Phi.createDividedCanvas(shouldWrite, shouldSave);
	}

	void write(bool hSigBack, bool hSig, bool hBack)
	{
		if (hSigBack == true)
		{
			this->Pt .hSigBack->Write();
			this->Eta.hSigBack->Write();
			this->Phi.hSigBack->Write();
		}

		if (hSig == true)
		{
			this->Pt .hSig->Write();
			this->Eta.hSig->Write();
			this->Phi.hSig->Write();
		}

		if (hBack == true)
		{
			this->Pt .hBack->Write();
			this->Eta.hBack->Write();
			this->Phi.hBack->Write();
		}
	}
};

//Select particles, draws and save histograms
void generateHistograms()
{
	TFile *file0 = TFile::Open("../data_histoall.root");		//Opens the file
	TTree *TreePC = (TTree*)file0->Get("demo/PlotControl");		//Opens TTree of file
	TTree *TreeAT = (TTree*)file0->Get("demo/AnalysisTree");	//Opens TTree of file
	
	//Create variables for PlotControl
	Double_t 	ProbeMuon_Pt;
	Double_t 	ProbeMuon_Eta;
	Double_t 	ProbeMuon_Phi;
	Double_t 	TagMuon_Pt;
	Double_t 	TagMuon_Eta;
	Double_t 	TagMuon_Phi;
	Double_t 	InvariantMass;

	//Create variables for AnalysisTree
	int PassingProbeTrackingMuon;
	int PassingProbeStandAloneMuon;
	int PassingProbeGlobalMuon;

	//Jpsi constants (PDG values)
	const double M_JPSI = 3.097;
	const double W_JPSI = 0.010;

	//Assign variables
	TreePC->SetBranchAddress("ProbeMuon_Pt",				&ProbeMuon_Pt);
	TreePC->SetBranchAddress("ProbeMuon_Eta",				&ProbeMuon_Eta);
	TreePC->SetBranchAddress("ProbeMuon_Phi",				&ProbeMuon_Phi);
	TreePC->SetBranchAddress("TagMuon_Pt",					&TagMuon_Pt);
	TreePC->SetBranchAddress("TagMuon_Eta",					&TagMuon_Eta);
	TreePC->SetBranchAddress("TagMuon_Phi",					&TagMuon_Phi);
	TreePC->SetBranchAddress("InvariantMass",				&InvariantMass);
	TreeAT->SetBranchAddress("PassingProbeTrackingMuon",	&PassingProbeTrackingMuon);
	TreeAT->SetBranchAddress("PassingProbeStandAloneMuon",	&PassingProbeStandAloneMuon);
	TreeAT->SetBranchAddress("PassingProbeGlobalMuon",		&PassingProbeGlobalMuon);

	//Create a object for Invariant Mass
	InvariantMassClass MassAll;
	MassAll.defineNumbers(240, 2.8, 3.4);
	MassAll.createMassHistogram();

	//Create a object
	TagAndProbe Tag;
	Tag.define("Tag");
	Tag.defineHistogramsTexts();
	Tag.defineHistogramsNumbers();
	Tag.createSigBackHistograms();
	Tag.createBackHistograms();

	TagAndProbe Probe;
	Probe.define("Probe");
	Probe.defineHistogramsTexts();
	Probe.defineHistogramsNumbers();
	Probe.createSigBackHistograms();
	Probe.createBackHistograms();


	//Loop between the components
	for (int i = 0; i < TreePC->GetEntries(); i++)
	{
		//Gets the entry from TTree
		TreePC->GetEntry(i);
		TreeAT->GetEntry(i);

		//Accepted particles
		if (TagMuon_Pt > 7.0 && abs(TagMuon_Eta) <= 2.4)
		{
			//Passing or failing
			if (PassingProbeTrackingMuon && !PassingProbeStandAloneMuon && !PassingProbeGlobalMuon)
			{
				//Fill invariant mass histogram
				MassAll.hMass->Fill(InvariantMass);

				//if is inside signal region
				if (fabs(InvariantMass - M_JPSI) < W_JPSI*10.0)
				{
					Probe.fillSigBackHistograms(ProbeMuon_Pt, 	ProbeMuon_Eta, 	ProbeMuon_Phi);
					Tag  .fillSigBackHistograms(TagMuon_Pt, 	TagMuon_Eta, 	TagMuon_Phi);
					
					//Count events
					MassAll.count_signalRegion++;
				}

				//If is inside sideband region
				if (fabs(InvariantMass - M_JPSI) > W_JPSI*10.5 && fabs(InvariantMass - M_JPSI) < W_JPSI*20.5)
				{
					//Adds to histograms
					Probe.fillBackHistograms(ProbeMuon_Pt, 	ProbeMuon_Eta, 	ProbeMuon_Phi);
					Tag  .fillBackHistograms(TagMuon_Pt, 	TagMuon_Eta, 	TagMuon_Phi);

					//Count events
					MassAll.count_sidebandRegion++;
				}
			}
		}
	}

	//Number of particles in signal and uncertain
	MassAll.sidebandCalculus();

	//Create signal histograms
	Probe.createSigHistograms();
	Tag.createSigHistograms();

	//-------------------------------------
	// Canvas
	//-------------------------------------

	//Create file root to store generated files
	TFile *generatedFile = TFile::Open("../generated_hist.root","RECREATE");
	generatedFile->mkdir("canvas/");
	generatedFile->   cd("canvas/");

	//Create canvas
	MassAll.createCanvas(true, true);
	//Probe.createDividedCanvas(true, true);
	//Tag.createDividedCanvas(true, true);

	//Debug
	cout << endl;
	cout << "Candidates by sideband subtraction" << endl;
	cout << "#Tree Entries    = " 	<< TreePC->GetEntries() 		<< endl;
	cout << "#Signal   region = "	<< MassAll.count_sidebandRegion	<< endl;
	cout << "#Sideband region = "	<< MassAll.count_sidebandRegion	<< endl;
	cout << "#Signal          = " 	<< MassAll.S					<< endl;
	cout << endl;

	//Integrate function to get number of particles in it
	cout << endl;
	cout << "Checking histograms number inconsistency (should be 0)" << endl;
	cout << "#Probe Pt  = " << Probe.Pt .hSigBack->GetEntries()  	- Probe.Pt .hSig->GetEntries()  - Probe.Pt .hBack->GetEntries()	<< endl;
	cout << "#Probe Eta = " << Probe.Eta.hSigBack->GetEntries() 	- Probe.Eta.hSig->GetEntries() 	- Probe.Eta.hBack->GetEntries()	<< endl;
	cout << "#Probe Phi = " << Probe.Phi.hSigBack->GetEntries() 	- Probe.Phi.hSig->GetEntries() 	- Probe.Phi.hBack->GetEntries()	<< endl;
	cout << "#Tag   Pt  = " << Tag.Pt .hSigBack->GetEntries()  		- Tag.Pt .hSig->GetEntries()  	- Tag.Pt .hBack->GetEntries()  	<< endl;
	cout << "#Tag   Eta = " << Tag.Eta.hSigBack->GetEntries() 		- Tag.Eta.hSig->GetEntries() 	- Tag.Eta.hBack->GetEntries() 	<< endl;
	cout << "#Tag   Phi = " << Tag.Phi.hSigBack->GetEntries() 		- Tag.Phi.hSig->GetEntries() 	- Tag.Phi.hBack->GetEntries() 	<< endl;

	//Save histograms
	generatedFile->mkdir("histograms/");
	generatedFile->   cd("histograms/");

	Tag.write(true, true, true);
	Probe.write(true, true, true);

	//Close files
	generatedFile->Close();
}

/*
//Estimates efficiency
void efficiencyOldMethod()
{
	//Opens the file
	TFile *generatedFile = TFile::Open("../generated_hist.root","UPDATE");

	//Import histograms
	TH1D *hPtSigBack 	= (TH1D*)generatedFile->Get("histograms/ProbeMuon_PtSigBack");
	TH1D *hPtSig 		= (TH1D*)generatedFile->Get("histograms/ProbeMuon_PtSig");
	TH1D *hEtaSigBack 	= (TH1D*)generatedFile->Get("histograms/ProbeMuon_EtaSigBack");
	TH1D *hEtaSig 		= (TH1D*)generatedFile->Get("histograms/ProbeMuon_EtaSig");
	TH1D *hPhiSigBack 	= (TH1D*)generatedFile->Get("histograms/ProbeMuon_PhiSigBack");
	TH1D *hPhiSig 		= (TH1D*)generatedFile->Get("histograms/ProbeMuon_PhiSig");

	//Efficiency calculation for Pt
	TH1D* hPtEff  = (TH1D*) hPtSig->Clone("ProbePt_Efficiency");
	hPtEff->SetTitle("Transversal Momentum Efficiency for Probe");
	hPtEff->GetYaxis()->SetTitle("Efficiency");
	hPtEff->Divide(hPtSig, hPtSigBack, 1.0, 1.0, "B");
	hPtEff->SetLineWidth(2);
	hPtEff->SetLineColor(kRed);
	hPtEff->SetMarkerStyle(21);
	hPtEff->SetMarkerSize(0.5);
	hPtEff->SetMarkerColor(kRed);
	hPtEff->SetMaximum(1);
	hPtEff->SetMinimum(0);
	TCanvas *c1 = new TCanvas("ProbePt_Efficiency","Probe Pt Efficiency", 800, 600);
	c1->SetTopMargin(0.07);
	c1->SetLeftMargin(0.12);
	c1->SetTicky(2);
	hPtEff->Draw();

	//Efficiency calculation for Eta
	TH1D* hEtaEff  = (TH1D*) hEtaSig->Clone("ProbeEta_Efficiency");
	hEtaEff->SetTitle("Pseudorapidity Efficiency for Probe");
	hEtaEff->GetYaxis()->SetTitle("Efficiency");
	hEtaEff->Divide(hEtaSig, hEtaSigBack, 1.0, 1.0, "B");
	hEtaEff->SetLineWidth(2);
	hEtaEff->SetLineColor(kRed);
	hEtaEff->SetMarkerStyle(21);
	hEtaEff->SetMarkerSize(0.5);
	hEtaEff->SetMarkerColor(kRed);
	hEtaEff->SetMaximum(1);
	hEtaEff->SetMinimum(0);
	TCanvas *c2 = new TCanvas("ProbeEta_Efficiency","Probe Eta Efficiency", 800, 600);
	c2->SetTopMargin(0.07);
	c2->SetLeftMargin(0.12);
	c2->SetTicky(2);
	hEtaEff->Draw();

	//Efficiency calculation for Phi
	TH1D* hPhiEff  = (TH1D*) hPhiSig->Clone("ProbePhi_Efficiency");
	hPhiEff->SetTitle("Phi Efficiency for Probe");
	hPhiEff->GetYaxis()->SetTitle("Efficiency");
	hPhiEff->Divide(hPhiSig, hPhiSigBack, 1.0, 1.0, "B");
	hPhiEff->SetLineWidth(2);
	hPhiEff->SetLineColor(kRed);
	hPhiEff->SetMarkerStyle(21);
	hPhiEff->SetMarkerSize(0.5);
	hPhiEff->SetMarkerColor(kRed);
	hPhiEff->SetMaximum(1);
	hPhiEff->SetMinimum(0);
	TCanvas *c3 = new TCanvas("ProbePhi_Efficiency","Probe Phi Efficiency", 800, 600);
	c3->SetTopMargin(0.07);
	c3->SetLeftMargin(0.12);
	c3->SetTicky(2);
	hPhiEff->Draw();



	//Not show frame
	gStyle->SetOptStat(0);

	//Saves as image
	c1->SaveAs("../PtProbe_Efficiency.png");
	c2->SaveAs("../EtaProbe_Efficiency.png");
	c3->SaveAs("../PhiProbe_Efficiency.png");

	//Saves new histograms and canvas in file
	generatedFile->Delete("efficiency/");
	generatedFile->mkdir("efficiency/canvas/");
	generatedFile->cd("efficiency/canvas/");
	c1->Write("",TObject::kOverwrite);
	c2->Write("",TObject::kOverwrite);
	c3->Write("",TObject::kOverwrite);
	generatedFile->mkdir("efficiency/histograms/");
	generatedFile->cd("efficiency/histograms/");
	hPtEff->Write("",TObject::kOverwrite);
	hEtaEff->Write("",TObject::kOverwrite);
	hPhiEff->Write("",TObject::kOverwrite);
}
*/

//Creates a efficiency plot with histograms
TEfficiency *efficiencyPlot(TH1D *hPass, TH1D *hTotal, const char *name, const char *title, bool shouldWrite = false)
{
	//Creates TEfficiency object
	TEfficiency* pEff = 0;

	//Set Y axis title for efficiency plot
	hTotal->GetYaxis()->SetTitle("Efficiency");

	//Check if are valid and consistent histograms
	if(TEfficiency::CheckConsistency(*hPass, *hTotal))
	{
		//Fills histogram
		pEff = new TEfficiency(*hPass, *hTotal);
	}

	//Set plot config
	pEff->SetTitle(title);
	pEff->SetLineWidth(2);
	pEff->SetLineColor(kBlack);
	pEff->SetMarkerStyle(21);
	pEff->SetMarkerSize(0.5);
	pEff->SetMarkerColor(kBlack);

	//Set range in y axis
	pEff->Draw(); 
	gPad->Update(); 
	auto graph = pEff->GetPaintedGraph(); 
	graph->SetMinimum(0.0);
	graph->SetMaximum(1.2);
	if (strcmp(name, "TagMuon_PhiEfficiency") == 0 || strcmp(name, "ProbeMuon_PhiEfficiency") == 0)
	{
		graph->SetMinimum(0.5);
		graph->SetMaximum(1.0);
	} 
	gPad->Update();

	//Set x range
	if (strcmp(name, "ProbeMuon_PtEfficiency") == 0 || strcmp(name, "TagMuon_PtEfficiency") == 0)
	{
		pEff->GetPaintedGraph()->GetHistogram()->GetXaxis()->SetRange(0.,40.);
	}

	//Writes in file
	if (shouldWrite == true)
	{
		pEff->Write("",TObject::kOverwrite);
	}

	//return
	return pEff;
}

//Creates canvas for efficiency plots
TCanvas *createEfficiencyCanvas(TEfficiency* pEff, const char *canvasName, const char *title, bool shouldWrite = false, const char *saveAs = "")
{
	//Draw on canvas
	TCanvas *c1 = new TCanvas(canvasName, title, 800, 600);
	c1->SetRightMargin(0.05);
	pEff->Draw();

	//Writes in file
	if (shouldWrite == true)
	{
		c1->Write("",TObject::kOverwrite);
	}

	//If should save
	if (strcmp(saveAs, "") != 0)
	{
		//Saves as image
		c1->SaveAs(saveAs);
	} 

	//return
	return c1;
}

//Estimates efficiency
void efficiency()
{
	//Opens the file
	TFile *generatedFile = TFile::Open("../generated_hist.root","UPDATE");

	//Import Probe histograms
	TH1D *hProbePtSigBack 	= (TH1D*)generatedFile->Get("histograms/ProbeMuon_PtSigBack");
	TH1D *hProbePtSig 		= (TH1D*)generatedFile->Get("histograms/ProbeMuon_PtSig");
	TH1D *hProbeEtaSigBack 	= (TH1D*)generatedFile->Get("histograms/ProbeMuon_EtaSigBack");
	TH1D *hProbeEtaSig 		= (TH1D*)generatedFile->Get("histograms/ProbeMuon_EtaSig");
	TH1D *hProbePhiSigBack 	= (TH1D*)generatedFile->Get("histograms/ProbeMuon_PhiSigBack");
	TH1D *hProbePhiSig 		= (TH1D*)generatedFile->Get("histograms/ProbeMuon_PhiSig");
	//Import Tag histograms
	TH1D *hTagPtSigBack 	= (TH1D*)generatedFile->Get("histograms/TagMuon_PtSigBack");
	TH1D *hTagPtSig 		= (TH1D*)generatedFile->Get("histograms/TagMuon_PtSig");
	TH1D *hTagEtaSigBack 	= (TH1D*)generatedFile->Get("histograms/TagMuon_EtaSigBack");
	TH1D *hTagEtaSig 		= (TH1D*)generatedFile->Get("histograms/TagMuon_EtaSig");
	TH1D *hTagPhiSigBack 	= (TH1D*)generatedFile->Get("histograms/TagMuon_PhiSigBack");
	TH1D *hTagPhiSig 		= (TH1D*)generatedFile->Get("histograms/TagMuon_PhiSig");

	//Deletes old dir and creates another
	generatedFile->Delete("efficiency/");
	generatedFile->mkdir("efficiency/plots/");
	generatedFile->cd("efficiency/plots/");

	//Creates efficiency plots
	TEfficiency* pProbePtEff  = efficiencyPlot(hProbePtSig,  hProbePtSigBack,  	"ProbeMuon_PtEfficiency",  "Transversal Momentum Efficiency for Probe", true);
	TEfficiency* pProbeEtaEff = efficiencyPlot(hProbeEtaSig, hProbeEtaSigBack, 	"ProbeMuon_EtaEfficiency", "Pseudorapidity Efficiency for Probe", 		true);
	TEfficiency* pProbePhiEff = efficiencyPlot(hProbePhiSig, hProbePhiSigBack, 	"ProbeMuon_PhiEfficiency", "Angle Efficiency for Probe", 				true);
	TEfficiency* pTagPtEff    = efficiencyPlot(hTagPtSig,  	 hTagPtSigBack,  	"TagMuon_PtEfficiency",    "Transversal Momentum Efficiency for Tag", 	true);
	TEfficiency* pTagEtaEff   = efficiencyPlot(hTagEtaSig, 	 hTagEtaSigBack, 	"TagMuon_EtaEfficiency",   "Pseudorapidity Efficiency for Tag", 		true);
	TEfficiency* pTagPhiEff   = efficiencyPlot(hTagPhiSig, 	 hTagPhiSigBack, 	"TagMuon_PhiEfficiency",   "Angle Efficiency for Tag",					true);

	//Saves new histograms and canvas in file
	generatedFile->mkdir("efficiency/histograms/");
	generatedFile->cd("efficiency/histograms/");

	//Create canvas for others
	createEfficiencyCanvas(pProbePtEff,   "ProbeMuon_PtEfficiency",  "Transversal Momentum Efficiency for Probe", 	true,	"../PtProbe_Efficiency.png");
	createEfficiencyCanvas(pProbeEtaEff,  "ProbeMuon_EtaEfficiency", "Pseudorapidity Efficiency for Probe", 		true,	"../EtaProbe_Efficiency.png");
	createEfficiencyCanvas(pProbePhiEff,  "ProbeMuon_PhiEfficiency", "Angle Efficiency for Probe", 					true,	"../PhiProbe_Efficiency.png");
	createEfficiencyCanvas(pTagPtEff,     "TagMuon_PtEfficiency", 	 "Transversal Momentum Efficiency for Tag", 	true,	"../PtTag_Efficiency.png");
	createEfficiencyCanvas(pTagEtaEff,    "TagMuon_EtaEfficiency",	 "Pseudorapidity Efficiency for Tag", 			true,	"../EtaTag_Efficiency.png");
	createEfficiencyCanvas(pTagPhiEff,    "TagMuon_PhiEfficiency",	 "Angle Efficiency for Tag", 					true,	"../PhiTag_Efficiency.png");
}

//Call functions
void boss()
{
	generateHistograms();
	//efficiencyOldMethod();
	//efficiency();
}