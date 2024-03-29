#include <vector>
#include <cmath>
#include <iostream>
#include <string>

#include <TCanvas.h>
#include <TGraphErrors.h>
#include <TF1.h>
#include <TStyle.h>
#include <TAxis.h>
#include <TMath.h>
#include <TLatex.h>
#include <TLegend.h>

// definiamo valori costanti utili nel programma
#define LR      0.800010
#define ERR_LR  0.000010
#define G_T     9.8056
#define ERR_G_T 0.0001

struct x_values{
    double xpos[2];
    double xneg[2];        
};

void print_mmsg(std::string mmsg){
    std::cout << std::endl 
        << " **********" << std::endl
        << "    " << mmsg << std::endl
        << " **********" << std::endl
        << std::endl;
}

void print_stat(TF1* _f){
    std::cout << std::endl
        << "** " << "CHI2 / NDF ( PROB. ) " 
        << _f->GetChisquare() << " / " << _f->GetNDF() << " ( " << _f->GetProb() << " )"
        << std::endl << std::endl;
}

x_values isocrono_x(Double_t* params_1, const Double_t* err_params_1, 
                    Double_t* params_2, const Double_t* err_params_2){
    x_values x;
    // a0+a1*x+a2*x^2
    // non usare valori assoluti per a0, a1 e a2 per evitare di ottenere valore di isocronia errato.
    double a2 = params_1[0]-params_2[0], err_a2 = abs(err_params_1[0])+abs(err_params_2[0]);
    double a1 = params_1[1]-params_2[1], err_a1 = abs(err_params_1[1])+abs(err_params_2[1]);
    double a0 = params_1[2]-params_2[2], err_a0 = abs(err_params_1[2])+abs(err_params_2[2]);

    double delta = pow(a1, 2) - 4*a0*a2;

    if(delta<0){
        std::cout << "delta is negative!" << std::endl;
        return {{0,0}, {0,0}};
    }

    x.xpos[0] = (-a1+sqrt(delta))/(2*a0);
    x.xneg[0] = (-a1-sqrt(delta))/(2*a0);

    double dda_sqr_pos = pow(((-1)*a2/(a0*sqrt(delta))) - ((-a1 + sqrt(delta))/(2*a0*a0)), 2);
    double ddb_sqr_pos = pow((-1 + a1/sqrt(delta))/(2*a0), 2);
    double ddc_sqr_pos = pow(-1/sqrt(delta), 2);
    
    // errore su xpos
    x.xpos[1] = sqrt((dda_sqr_pos*err_a0*err_a0) + (ddb_sqr_pos*err_a1*err_a1) + (ddc_sqr_pos*err_a2*err_a2));

    double dda_sqr_neg = pow((a1*a1 - 2*a0*a2 + a1*sqrt(delta))/(2*a0*a0*sqrt(delta)), 2);
    double ddb_sqr_neg = pow((-1 - (a1/sqrt(delta)))/(2*a0), 2);
    double ddc_sqr_neg = pow(1/sqrt(delta), 2);

    // err su xneg
    x.xneg[1] = sqrt((dda_sqr_neg*err_a0*err_a0) + (ddb_sqr_neg*err_a1*err_a1) + (ddc_sqr_neg*err_a2*err_a2));

    return x;
}

double* get_isoX(x_values x, TGraphErrors* _g){
    
    double* _x = new double[2];

    if(x.xpos[0]>TMath::MinElement(_g->GetN(), _g->GetX())){
        _x[0] = x.xpos[0]; _x[1] = x.xpos[1];
    }else{
        _x[0] = x.xneg[0]; _x[1] = x.xneg[1];
    }
    
    return _x;

}

double get_err_T(double x_iso, TGraphErrors* g1, TGraphErrors* g2){

    double Terr_iso_1 = 0;
    double Terr_iso_2 = 0;
    double range = abs(g1->GetX()[0]-x_iso);

    for(int i=0; i<g1->GetN(); i++){
        // std::cout << i << " " << abs(x_iso-g1.GetX()[i]) << " x: " << g1.GetX()[i] << std::endl;
        // algorimo per identificazione dei valori di T1 T2 più
        // vicini al valore di isocronia

        if((abs(x_iso-g1->GetX()[i])<range)){
            Terr_iso_1 = g1->GetErrorY(i);
            Terr_iso_2 = g2->GetErrorY(i);
            range = abs(x_iso-g1->GetX()[i]);
        }
    }

    return sqrt(pow(Terr_iso_1, 2)+pow(Terr_iso_2, 2));
}

bool compatible(double G1, double errG1,
                double G2, double errG2){
    double abs_values = abs(G2-G1);
    double err_abs_val = 3*sqrt(pow(errG1, 2) + pow(errG2, 2));
    if(abs_values<err_abs_val){
        return true;
    }
    return false;
}

double get_g(double T_iso){
    double g_value = 10;
    g_value = 4 * pow(M_PI, 2) * LR / pow(T_iso, 2);
    return g_value;
}

double get_gerr(double T_iso, double Terr_iso){
    double g_err = 0;
    double ddT_sqr = pow( - (8 * LR * pow(M_PI, 2) / pow(T_iso, 3)), 2);
    double ddlr_sqr = pow(4*M_PI/pow(T_iso, 2), 2);
    g_err = sqrt(ddT_sqr * pow(Terr_iso, 2) + ddlr_sqr * pow(ERR_LR, 2));
    return g_err;
}

void kater_plot(bool header = false, bool fast = false){

    gStyle->SetFrameLineWidth(0);

    gStyle->SetTextFont(42);

    TCanvas* c1 = new TCanvas("c1", "", 600, 500);
    c1->SetMargin(0.16, 0.06, 0.12, 0.06);
    // c1.SetGrid();

    TGraphErrors* g1 = new TGraphErrors("../dati/computed_T1_x.txt"); g1->SetName("g1");
    g1->SetMarkerStyle(4);
    g1->SetLineColor(kBlack);
    TGraphErrors* g2 = new TGraphErrors("../dati/computed_T2_x.txt"); g2->SetName("g2");
    g2->SetLineColor(kRed);
    g2->SetMarkerStyle(4);
    g2->SetMarkerColor(kRed);

    TF1* f1 = new TF1("f1", "pol2"); // a1 = p0, b1 = p1, c1 = p2;
    f1->SetLineColor(kBlack);
    f1->SetParameters(1, 0, 5);
    TF1* f2 = new TF1("f2", "pol2"); // a2 = p0, b2 = p1, c2 = p2;
    f2->SetParameters(2, -1, 1);
    f2->SetLineColor(kRed);

    g1->SetTitle("");
    g1->GetXaxis()->SetTitle("Posizione x_{b} della massa M_{b} (m)");
    g1->GetYaxis()->SetTitle("Periodo T (s)");
    g1->GetXaxis()->SetTitleOffset(0.85);
    g1->GetXaxis()->SetTitleSize(0.06);
    g1->GetYaxis()->SetTitleSize(0.06);
    g1->GetXaxis()->CenterTitle();
    g1->GetYaxis()->CenterTitle();

    print_mmsg("PROCESSING G1...");

    g1->Draw("ap");
    g1->Fit("f1", "E");

    print_stat(f1);

    print_mmsg("PROCESSING G2...");

    g2->Draw("p");
    g2->Fit("f2", "E");

    print_stat(f2);

    print_mmsg("RESULTS ...");

    Double_t* par_1 = f1->GetParameters();
    const Double_t* par_1_err = f1->GetParErrors();
    Double_t* par_2 = f2->GetParameters();
    const Double_t* par_2_err = f2->GetParErrors();

    double* x_iso = get_isoX(isocrono_x(par_1, par_1_err, par_2, par_2_err), g1);
    double T_iso = f1->Eval(x_iso[0]);
    double Terr_iso = get_err_T(x_iso[0], g1, g2);

    std::cout << "x* = " << x_iso[0] << " +/- " << x_iso[1] << std::endl;
    std::cout << "T* = " << T_iso << " +/- " << Terr_iso << std::endl;

    double g_s = get_g(T_iso);
    double gerr_s = get_gerr(T_iso, Terr_iso);

    if(compatible(g_s, gerr_s, G_T, ERR_G_T)){
        print_mmsg("COMPATIBLE VALUES");
        std::cout << "g = " << g_s << " +/- " <<  gerr_s << std::endl;
    } else {
        print_mmsg("NON-COMPATIBLE VALUES...");
        std::cout << "g = " << g_s << " +/- " <<  gerr_s << std::endl;
        std::cout << "Delta = abs( abs( gt - gs ) - 3*sqrt( err_gt^2 + err_gs^2 ) ) = " 
            << abs(abs(g_s-G_T)-3*sqrt(pow(gerr_s, 2) + pow(ERR_G_T, 2))) << std::endl;
    }

    if(!fast){
        std::string ss_1="#chi^{2}/ndf (prob.) = "
            +std::to_string(f1->GetChisquare())+"/"
            +std::to_string(f1->GetNDF())
            +" ("+std::to_string(f1->GetProb())+")";
        TLatex sl_1;
        sl_1.SetTextSize(0.035);
        sl_1.SetTextColor(kBlack);
        sl_1.DrawLatexNDC(0.50, 0.28, ss_1.c_str());
        
        
        std::string ss_2="#chi^{2}/ndf (prob.) = "
            +std::to_string(f2->GetChisquare())+"/"
            +std::to_string(f2->GetNDF())
            +" ("+std::to_string(f2->GetProb())+")";
        TLatex sl_2;
        sl_1.SetTextSize(0.035);
        sl_1.SetTextColor(kRed);
        sl_1.DrawLatexNDC(0.50, 0.24, ss_2.c_str());
    }
    

    if(!header){
        TLatex header;
        header.SetTextSize(0.055);
        header.DrawLatexNDC(0.25, 0.85, "#bf{Dati dalle tabelle 1 e 2}");
    }

    TLegend* leg = new TLegend(0.25, 0.65, 0.5, 0.8);
    leg->AddEntry("g1", "Periodi T1");
    leg->AddEntry("g2", "Periodi T2");
    leg->SetBorderSize(0);
    leg->Draw();

    c1->SaveAs("../fig/kater_plot.pdf");
    c1->Draw();

    delete[] x_iso;
    return;
}

int main(){
    kater_plot();
}
