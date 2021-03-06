#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "main.h"

#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QClipboard>
#include <QWhatsThis>
#include <QDebug>
#include <cmath>
#include <string>
#include <sstream>
#include <fstream>

using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    conf = new ConfigContainer();

    LogHelper::clearLog();
    LogHelper::writeLog("UI launched...");

    m_stylehelper = new StyleHelper(this);
    m_appwrapper = new AppConfigWrapper(m_stylehelper);

    m_appwrapper->loadAppConfig();
    conf->setConfigMap(ConfigIO::readFile(m_appwrapper->getPath()));
    LoadConfig();

    QMenu *menu = new QMenu();
    menu->addAction("Reload Viper", this,SLOT(Restart()));
    menu->addAction("Context Help", this,[](){QWhatsThis::enterWhatsThisMode();});
    menu->addAction("Load from file", this,SLOT(LoadExternalFile()));
    menu->addAction("Save to file", this,SLOT(SaveExternalFile()));
    menu->addAction("View Logs", this,SLOT(OpenLog()));

    ui->toolButton->setMenu(menu);
    QMenu *menuC = new QMenu();
    menuC->addAction("Slight", this,[this](){ ui->colmpreset->setText("Slight"); ColmPresetSelectionUpdated();});
    menuC->addAction("Level 1", this,[this](){ ui->colmpreset->setText("Level 1"); ColmPresetSelectionUpdated();});
    menuC->addAction("Level 2", this,[this](){ ui->colmpreset->setText("Level 2"); ColmPresetSelectionUpdated();});
    menuC->addAction("Level 3", this,[this](){ ui->colmpreset->setText("Level 3"); ColmPresetSelectionUpdated();});
    menuC->addAction("Level 4", this,[this](){ ui->colmpreset->setText("Level 4"); ColmPresetSelectionUpdated();});
    menuC->addAction("Level 5", this,[this](){ ui->colmpreset->setText("Level 5"); ColmPresetSelectionUpdated();});
    menuC->addAction("Level 6", this,[this](){ ui->colmpreset->setText("Level 6"); ColmPresetSelectionUpdated();});
    menuC->addAction("Level 7", this,[this](){ ui->colmpreset->setText("Level 7"); ColmPresetSelectionUpdated();});
    menuC->addAction("Extreme", this,[this](){ ui->colmpreset->setText("Extreme"); ColmPresetSelectionUpdated();});
    ui->colmpreset->setMenu(menuC);
    m_stylehelper->SetStyle();
    ConnectActions();
}

MainWindow::~MainWindow()
{
    delete ui;
}

//---Dialogs/Buttons
void MainWindow::DialogHandler(){
    if(sender() == ui->conv_select){
        EnableConvolverButton(false);
        (new Convolver(this))->show();
    }
    else if(sender() == ui->set){
        if(settingsdlg_enabled){
            EnableSettingButton(false);
            (new settings(this))->show();
        }
    }
    else if(sender() == ui->cpreset){
        if(presetdlg_enabled){
            EnablePresetButton(false);
            (new Preset(this))->show();
        }
    }
    else if(sender() == ui->set){
        if(settingsdlg_enabled){
            EnableSettingButton(false);
            (new settings(this))->show();
        }
    }
}
void MainWindow::OpenLog(){
    if(logdlg_enabled){
        EnableLogButton(false);
        (new class log(this))->show();
    }
}
void MainWindow::Reset(){
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,"Reset Configuration","Are you sure?",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        std::filebuf fb;
        fb.open (m_appwrapper->getPath().toUtf8().constData(),std::ios::out);
        std::ostream os(&fb);
        os << default_config;
        fb.close();

        conf->setConfigMap(ConfigIO::readFile(m_appwrapper->getPath()));
        LoadConfig();

        ApplyConfig();
    }
}
void MainWindow::DisableFX(){
    //Apply instantly
    if(!lockapply)ApplyConfig();
}
void MainWindow::EnableSettingButton(bool on){
    settingsdlg_enabled=on;
}
void MainWindow::EnablePresetButton(bool on){
    presetdlg_enabled=on;
}
void MainWindow::EnableConvolverButton(bool on){
    ui->conv_select->setEnabled(on);
}
void MainWindow::EnableLogButton(bool on){
    logdlg_enabled=on;
}

//---Reloader
void MainWindow::OnUpdate(bool ignoremode){
    //Will be called when slider has been moved, dynsys/eq preset set or spinbox changed
    if((m_appwrapper->getAutoFx() &&
        (ignoremode||m_appwrapper->getAutoFxMode()==0)) && !lockapply)
        ApplyConfig();
}
void MainWindow::OnRelease(){
    if((m_appwrapper->getAutoFx() &&
        m_appwrapper->getAutoFxMode()==1) && !lockapply)
        ApplyConfig();
}
void MainWindow::Restart(){
    if(m_appwrapper->getMuteOnRestart())system("pactl set-sink-mute 0 1");
    if(m_appwrapper->getGFix())system("killall -r glava");
    system("viper restart");
    if(m_appwrapper->getGFix())system("setsid glava -d >/dev/null 2>&1 &");
    if(m_appwrapper->getMuteOnRestart())system("pactl set-sink-mute 0 0");
}

//---User preset management
void MainWindow::LoadPresetFile(const QString& filename){
    const QString& src = filename;
    const QString dest = m_appwrapper->getPath();
    if (QFile::exists(dest))QFile::remove(dest);

    QFile::copy(src,dest);
    LogHelper::writeLog("Loading from " + filename+ " (main/loadpreset)");
    conf->setConfigMap(ConfigIO::readFile(m_appwrapper->getPath()));
    LoadConfig();

    ApplyConfig();
}
void MainWindow::SavePresetFile(const QString& filename){
    const QString src = m_appwrapper->getPath();
    const QString& dest = filename;
    if (QFile::exists(dest))QFile::remove(dest);

    QFile::copy(src,dest);
    LogHelper::writeLog("Saving to " + filename+ " (main/savepreset)");
}
void MainWindow::LoadExternalFile(){
    QString filename = QFileDialog::getOpenFileName(this,"Load custom audio.conf","","*.conf");
    if(filename=="")return;
    const QString& src = filename;
    const QString dest = m_appwrapper->getPath();
    if (QFile::exists(dest))QFile::remove(dest);

    QFile::copy(src,dest);
    LogHelper::writeLog("Loading from " + filename+ " (main/loadexternal)");
    conf->setConfigMap(ConfigIO::readFile(m_appwrapper->getPath()));
    LoadConfig();

    ApplyConfig();
}
void MainWindow::SaveExternalFile(){
    QString filename = QFileDialog::getSaveFileName(this,"Save current audio.conf","","*.conf");
    if(filename=="")return;
    QFileInfo fi(filename);
    QString ext = fi.suffix();
    if(ext!="conf")filename.append(".conf");

    const QString src = m_appwrapper->getPath();
    const QString dest = filename;
    if (QFile::exists(dest))QFile::remove(dest);

    QFile::copy(src,dest);
    LogHelper::writeLog("Saving to " + filename+ " (main/saveexternal)");
}

//---Config IO
void MainWindow::LoadConfig(){
    lockapply=true;
    ui->disableFX->setChecked(!conf->getBool("fx_enable"));
    ui->tubesim->setChecked(conf->getBool("tube_enable"));
    ui->colm->setChecked(conf->getBool("colm_enable"));
    ui->colmwide->setValue(conf->getInt("colm_widening"));
    ui->colmdepth->setValue(conf->getInt("colm_depth"));
    ui->colmmidimg->setValue(conf->getInt("colm_midimage"));
    ui->colmwide->setValue(conf->getInt("colm_widening"));
    ui->clarity->setChecked(conf->getBool("vc_enable"));
    ui->vcmode->setValue(conf->getInt("vc_mode"));
    ui->vclvl->setValue(conf->getInt("vc_level"));
    ui->vb->setChecked(conf->getBool("vb_enable"));
    ui->vbmode->setValue(conf->getInt("vb_mode"));
    ui->vbgain->setValue(conf->getInt("vb_gain"));
    ui->vbfreq->setValue(conf->getInt("vb_freq"));
    ui->vhp->setChecked(conf->getBool("vhe_enable"));
    ui->vhplvl->setValue(conf->getInt("vhe_level"));
    ui->diff->setChecked(conf->getBool("ds_enable"));
    ui->difflvl->setValue(conf->getInt("ds_level"));
    ui->reverb->setChecked(conf->getBool("reverb_enable"));
    ui->roomsize->setValue(conf->getInt("reverb_roomsize"));
    ui->roomwidth->setValue(conf->getInt("reverb_width"));
    ui->roomdamp->setValue(conf->getInt("reverb_damp"));
    ui->wet->setValue(conf->getInt("reverb_wet"));
    ui->dry->setValue(conf->getInt("reverb_dry"));
    ui->agc->setChecked(conf->getBool("agc_enable"));
    ui->gain->setValue(conf->getInt("agc_ratio"));
    ui->maxgain->setValue(conf->getInt("agc_maxgain"));
    ui->maxvol->setValue(conf->getInt("agc_volume"));
    ui->limiter->setValue(conf->getInt("lim_threshold"));
    ui->outputpan->setValue(conf->getInt("out_pan"));
    ui->outvolume->setValue(conf->getInt("out_volume"));
    ui->enable_eq->setChecked(conf->getBool("eq_enable"));
    ui->eq1->setValue(conf->getInt("eq_band1"));
    ui->eq2->setValue(conf->getInt("eq_band2"));
    ui->eq3->setValue(conf->getInt("eq_band3"));
    ui->eq4->setValue(conf->getInt("eq_band4"));
    ui->eq5->setValue(conf->getInt("eq_band5"));
    ui->eq6->setValue(conf->getInt("eq_band6"));
    ui->eq7->setValue(conf->getInt("eq_band7"));
    ui->eq8->setValue(conf->getInt("eq_band8"));
    ui->eq9->setValue(conf->getInt("eq_band9"));
    ui->eq10->setValue(conf->getInt("eq_band10"));
    ui->enable_comp->setChecked(conf->getBool("fetcomp_enable"));
    ui->m_gain->setChecked(conf->getBool("fetcomp_autogain"));
    ui->m_width->setChecked(conf->getBool("fetcomp_autoknee"));
    ui->m_attack->setChecked(conf->getBool("fetcomp_autoattack"));
    ui->m_release->setChecked(conf->getBool("fetcomp_autorelease"));
    ui->noclip->setChecked(conf->getBool("fetcomp_noclip"));
    ui->comp_thres->setValue(conf->getInt("fetcomp_threshold"));
    ui->compgain->setValue(conf->getInt("fetcomp_gain"));
    ui->compwidth->setValue(conf->getInt("fetcomp_kneewidth"));
    ui->comp_ratio->setValue(conf->getInt("fetcomp_ratio"));
    ui->compattack->setValue(conf->getInt("fetcomp_attack"));
    ui->comprelease->setValue(conf->getInt("fetcomp_release"));
    ui->a_adapt->setValue(conf->getInt("fetcomp_meta_adapt"));
    ui->a_crest->setValue(conf->getInt("fetcomp_meta_crest"));
    ui->a_maxatk->setValue(conf->getInt("fetcomp_meta_maxattack"));
    ui->a_maxrel->setValue(conf->getInt("fetcomp_meta_maxrelease"));
    ui->a_kneewidth->setValue(conf->getInt("fetcomp_meta_kneemulti"));
    ui->vcure->setChecked(conf->getBool("cure_enable"));
    ui->vcurelvl->setValue(conf->getInt("cure_level"));
    ui->ax->setChecked(conf->getBool("ax_enable"));
    ui->axmode->setValue(conf->getInt("ax_mode"));
    ui->vse->setChecked(conf->getBool("vse_enable"));
    ui->barkfreq->setValue(conf->getInt("vse_ref_bark"));
    ui->barkcon->setValue(conf->getInt("vse_bark_cons"));
    ui->conv->setChecked(conf->getBool("conv_enable"));
    ui->convcc->setValue(conf->getInt("conv_cc_level"));
    ui->dynsys->setChecked(conf->getBool("dynsys_enable"));
    ui->dyn_xcoeff1->setValue(conf->getInt("dynsys_xcoeff1"));
    ui->dyn_xcoeff2->setValue(conf->getInt("dynsys_xcoeff2"));
    ui->dyn_ycoeff1->setValue(conf->getInt("dynsys_ycoeff1"));
    ui->dyn_ycoeff2->setValue(conf->getInt("dynsys_ycoeff2"));
    ui->dyn_sidegain1->setValue(conf->getInt("dynsys_sidegain1"));
    ui->dyn_sidegain2->setValue(conf->getInt("dynsys_sidegain1"));
    ui->dyn_bassgain->setValue(conf->getInt("dynsys_bassgain"));

    UpdateAllUnitLabels();

    QString ir = conf->getString("conv_ir_path");
    if(ir.size() > 2){
        ir.remove(0,1); //remove double quotes
        ir.chop(1);
        activeirs = ir.toUtf8().constData();
        QFileInfo irsInfo(ir);
        ui->convpath->setText(irsInfo.baseName());
        ui->convpath->setCursorPosition(0);
    }
    lockapply=false;
}
void MainWindow::ApplyConfig(bool restart){
    conf->setValue("fx_enable",QVariant(!ui->disableFX->isChecked()));
    conf->setValue("tube_enable",QVariant(ui->tubesim->isChecked()));
    conf->setValue("colm_enable",QVariant(ui->colm->isChecked()));
    conf->setValue("colm_widening",QVariant(ui->colmwide->value()));
    conf->setValue("colm_depth",QVariant(ui->colmdepth->value()));
    conf->setValue("colm_midimage",QVariant(ui->colmmidimg->value()));
    conf->setValue("vc_enable",QVariant(ui->clarity->isChecked()));
    conf->setValue("vc_mode",QVariant(ui->vcmode->value()));
    conf->setValue("vc_level",QVariant(ui->vclvl->value()));
    conf->setValue("cure_enable",QVariant(ui->vcure->isChecked()));
    conf->setValue("ax_enable",QVariant(ui->ax->isChecked()));
    conf->setValue("cure_level",QVariant(ui->vcurelvl->value()));
    conf->setValue("ax_mode",QVariant(ui->axmode->value()));
    conf->setValue("vse_enable",QVariant(ui->vse->isChecked()));
    conf->setValue("vse_ref_bark",QVariant(ui->barkfreq->value()));
    conf->setValue("vse_bark_cons",QVariant(ui->barkcon->value()));
    conf->setValue("conv_enable",QVariant(ui->conv->isChecked()));
    conf->setValue("conv_cc_level",QVariant(ui->convcc->value()));
    conf->setValue("conv_ir_path",QVariant("\""+ QString::fromStdString(activeirs) + "\""));
    conf->setValue("dynsys_enable",QVariant(ui->dynsys->isChecked()));
    conf->setValue("dynsys_bassgain",QVariant(ui->dyn_bassgain->value()));
    conf->setValue("dynsys_sidegain1",QVariant(ui->dyn_sidegain1->value()));
    conf->setValue("dynsys_sidegain2",QVariant(ui->dyn_sidegain2->value()));
    conf->setValue("dynsys_xcoeff1",QVariant(ui->dyn_xcoeff1->value()));
    conf->setValue("dynsys_xcoeff2",QVariant(ui->dyn_xcoeff2->value()));
    conf->setValue("dynsys_ycoeff1",QVariant(ui->dyn_ycoeff1->value()));
    conf->setValue("dynsys_ycoeff2",QVariant(ui->dyn_ycoeff2->value()));
    conf->setValue("agc_enable",QVariant(ui->agc->isChecked()));
    conf->setValue("agc_ratio",QVariant(ui->gain->value()));
    conf->setValue("agc_maxgain",QVariant(ui->maxgain->value()));
    conf->setValue("agc_volume",QVariant(ui->maxvol->value()));
    conf->setValue("lim_threshold",QVariant(ui->limiter->value()));
    conf->setValue("out_pan",QVariant(ui->outputpan->value()));
    conf->setValue("out_volume",QVariant(ui->outvolume->value()));
    conf->setValue("vhe_enable",QVariant(ui->vhp->isChecked()));
    conf->setValue("vhe_level",QVariant(ui->vhplvl->value()));
    conf->setValue("ds_enable",QVariant(ui->diff->isChecked()));
    conf->setValue("ds_level",QVariant(ui->difflvl->value()));
    conf->setValue("reverb_enable",QVariant(ui->reverb->isChecked()));
    conf->setValue("reverb_roomsize",QVariant(ui->roomsize->value()));
    conf->setValue("reverb_width",QVariant(ui->roomwidth->value()));
    conf->setValue("reverb_damp",QVariant(ui->roomdamp->value()));
    conf->setValue("reverb_wet",QVariant(ui->wet->value()));
    conf->setValue("reverb_dry",QVariant(ui->dry->value()));
    conf->setValue("vb_enable",QVariant(ui->vb->isChecked()));
    conf->setValue("vb_freq",QVariant(ui->vbfreq->value()));
    conf->setValue("vb_gain",QVariant(ui->vbgain->value()));
    conf->setValue("vb_mode",QVariant(ui->vbmode->value()));
    conf->setValue("fetcomp_enable",QVariant(ui->enable_comp->isChecked()));
    conf->setValue("fetcomp_autoattack",QVariant(ui->m_attack->isChecked()));
    conf->setValue("fetcomp_autogain",QVariant(ui->m_gain->isChecked()));
    conf->setValue("fetcomp_autoknee",QVariant(ui->m_width->isChecked()));
    conf->setValue("fetcomp_autorelease",QVariant(ui->m_release->isChecked()));
    conf->setValue("fetcomp_noclip",QVariant(ui->noclip->isChecked()));
    conf->setValue("fetcomp_threshold",QVariant(ui->comp_thres->value()));
    conf->setValue("fetcomp_gain",QVariant(ui->compgain->value()));
    conf->setValue("fetcomp_kneewidth",QVariant(ui->compwidth->value()));
    conf->setValue("fetcomp_ratio",QVariant(ui->comp_ratio->value()));
    conf->setValue("fetcomp_attack",QVariant(ui->compattack->value()));
    conf->setValue("fetcomp_release",QVariant(ui->comprelease->value()));
    conf->setValue("fetcomp_meta_adapt",QVariant(ui->a_adapt->value()));
    conf->setValue("fetcomp_meta_crest",QVariant(ui->a_crest->value()));
    conf->setValue("fetcomp_meta_maxattack",QVariant(ui->a_maxatk->value()));
    conf->setValue("fetcomp_meta_maxrelease",QVariant(ui->a_maxrel->value()));
    conf->setValue("fetcomp_meta_kneemulti",QVariant(ui->a_kneewidth->value()));
    conf->setValue("eq_enable",QVariant(ui->enable_eq->isChecked()));
    conf->setValue("eq_band1",QVariant(ui->eq1->value()));
    conf->setValue("eq_band2",QVariant(ui->eq2->value()));
    conf->setValue("eq_band3",QVariant(ui->eq3->value()));
    conf->setValue("eq_band4",QVariant(ui->eq4->value()));
    conf->setValue("eq_band5",QVariant(ui->eq5->value()));
    conf->setValue("eq_band6",QVariant(ui->eq6->value()));
    conf->setValue("eq_band7",QVariant(ui->eq7->value()));
    conf->setValue("eq_band8",QVariant(ui->eq8->value()));
    conf->setValue("eq_band9",QVariant(ui->eq9->value()));
    conf->setValue("eq_band10",QVariant(ui->eq10->value()));

    ConfigIO::writeFile(m_appwrapper->getPath(),conf->getConfigMap());
    if(restart) Restart();
}

//---Predefined Presets
void MainWindow::EqPresetSelectionUpdated(){
    const int* preset = EQ::lookupEQPreset(ui->eqpreset->currentText());
    if(preset != nullptr) SetEQ(preset);
    else ResetEQ();
}
void MainWindow::DynsysPresetSelectionUpdated(){
    const int* data = EQ::lookupDynsysPreset(ui->dynsys_preset->currentText());
    if(data==nullptr)return;
    lockapply=true;
    ui->dyn_xcoeff1->setValue(data[0]);
    ui->dyn_xcoeff2->setValue(data[1]);
    ui->dyn_ycoeff1->setValue(data[2]);
    ui->dyn_ycoeff2->setValue(data[3]);
    ui->dyn_sidegain1->setValue(data[4]);
    ui->dyn_sidegain2->setValue(data[5]);
    lockapply=false;
    OnUpdate(true);
}
void MainWindow::ColmPresetSelectionUpdated(){
    QString selection = ui->colmpreset->text();
    const int* data = EQ::lookupColmPreset(selection);
    lockapply=true;
    ui->colmwide->setValue(data[0]);
    ui->colmdepth->setValue(data[1]);
    lockapply=false;
    OnUpdate(true);
}

//--Status
void MainWindow::UpdateUnitLabel(int d,QObject *alt){
    if(lockapply&&alt==nullptr)return;//Skip if lockapply-flag is set (when setting presets, ...)
    QObject* obj;

    if(alt==nullptr)obj = sender();
    else obj = alt;

    QString pre = "";
    QString post = "";
    if(obj==ui->vbmode){
        //Bass
        if(d==0) UpdateTooltipLabelUnit(obj,"Natural Bass",alt==nullptr);
        else if(d==1) UpdateTooltipLabelUnit(obj,"Pure Bass+",alt==nullptr);
        else if(d==2) UpdateTooltipLabelUnit(obj,"Subwoofer",alt==nullptr);
        else UpdateTooltipLabelUnit(obj,"Mode "+QString::number( d ),alt==nullptr);
    }
    else if(obj==ui->vcmode){
        //Clarity
        if(d==0) UpdateTooltipLabelUnit(obj,"Natural",alt==nullptr);
        else if(d==1) UpdateTooltipLabelUnit(obj,"OZone+",alt==nullptr);
        else if(d==2) UpdateTooltipLabelUnit(obj,"XHiFi",alt==nullptr);
        else UpdateTooltipLabelUnit(obj,"Mode "+QString::number( d ),alt==nullptr);
    }
    else if(obj==ui->gain){
        //AGC
        if(d < 50) UpdateTooltipLabelUnit(obj,"Very Slight (" + QString::number( d ) + ")",alt==nullptr);
        else if(d < 100) UpdateTooltipLabelUnit(obj,"Slight (" + QString::number( d )+")",alt==nullptr);
        else if(d < 300) UpdateTooltipLabelUnit(obj,"Moderate (" + QString::number( d )+")",alt==nullptr);
        else UpdateTooltipLabelUnit(obj,"Extreme (" + QString::number( d )+")",alt==nullptr);
    }
    else if(obj==ui->axmode){
        //AnalogX
        if(d==0) UpdateTooltipLabelUnit(obj,"Slight",alt==nullptr);
        else if(d==1) UpdateTooltipLabelUnit(obj,"Moderate",alt==nullptr);
        else if(d==2) UpdateTooltipLabelUnit(obj,"Extreme",alt==nullptr);
        else UpdateTooltipLabelUnit(obj,"Mode "+QString::number( d ),alt==nullptr);
    }
    else if(obj==ui->vcurelvl){
        //Cure+
        if(d==0) UpdateTooltipLabelUnit(obj,"Slight",alt==nullptr);
        else if(d==1) UpdateTooltipLabelUnit(obj,"Moderate",alt==nullptr);
        else if(d==2) UpdateTooltipLabelUnit(obj,"Extreme",alt==nullptr);
        else UpdateTooltipLabelUnit(obj,"Mode "+QString::number( d ),alt==nullptr);
    }
    //Dynsys
    else if(obj==ui->dyn_bassgain) UpdateTooltipLabelUnit(obj,QString::number( (d-100)/20 ) + "%",alt==nullptr);
    else if(obj==ui->eq1||obj==ui->eq2||obj==ui->eq3||obj==ui->eq4||obj==ui->eq5||obj==ui->eq6||obj==ui->eq7||obj==ui->eq8||obj==ui->eq9||obj==ui->eq10)
        ui->info->setText(MathFunctions::buildEqGainString(d));
    //Diff-Surround
    else if(obj==ui->difflvl)UpdateTooltipLabelUnit(obj,QString::number(translate(d,0,100,0,20))+"ms (" + QString::number(d) + "%)",alt==nullptr);
    //AGC
    else if(obj==ui->maxgain)UpdateTooltipLabelUnit(obj,QString::number((int)translate(d,100,800,1,8))+"x (" + QString::number(d) + ")",alt==nullptr);
    else if(obj==ui->maxvol)UpdateTooltipLabelUnit(obj,QString::number(roundf(translate(d,100,0,0,-30)*100)/100)+"dB (" + QString::number(d) + ")",alt==nullptr);
    //Bass
    else if(obj==ui->vbgain){
        long double x = d;
        long double in =1.472577725 * pow(10L,-18L) * pow(x,7L) - 3.011526005 * pow(10L,-15L) * pow(x,6L) + 2.29923043 * pow(10L,-12L) * pow(x,5L) - 9.530124502 * pow(10L,-10L) * pow(x,4L) + 3.960377639 * pow(10L,-7L) * pow(x,3L) - 1.965034894 * pow(10L,-4L) * pow(x,2L) + 7.693150538 * pow(10L,-2L) * x + 1.965508847 * pow(10L,-2L);
        UpdateTooltipLabelUnit(obj,QString::number(roundf(in*100)/100)+"dB (" + QString::number(d) + ")",alt==nullptr);
    }
    //Clarity
    else if(obj==ui->vclvl)UpdateTooltipLabelUnit(obj,QString::number(roundf(translate(d,0,450,0,14.8)*100)/100)+"dB (" + QString::number(d) + ")",alt==nullptr);
    //Volume
    else if(obj==ui->outvolume){
        long double x = d;
        //Note:
        //V4A uses hardcoded arrays to display the output dB values when updating the slider in the app;
        //Although this makes it easier to implement, it is not really responsive.
        //Unlike the other sliders, this one increases exponentially rather than linearly, because of that I had to recreate a formula from the values in the v4a app, to calculate an accurate dB value (using polynomial regression; this might not be the most efficient solution).
        long double in = -7.095691001L* pow(10L,-13L) * pow(x,8L) + 3.130488467L* pow(10L,-10L) * pow(x,7L) - 5.667388779* pow(10L,-8L) * pow(x,6L) + 5.394863197L* pow(10L,-6L) * pow(x,5L) - 2.864305503L* pow(10L,-4L) * pow(x,4L)+ 8.264191247L* pow(10L,-3L) * pow(x,3L) - 1.218006784L* pow(10L,-1L) * pow(x,2L)+ 1.529341362L * x - 40.00317088L;
        UpdateTooltipLabelUnit(obj,QString::number(roundf(in*100)/100)+"dB (" + QString::number(d) + ")",alt==nullptr);
    }
    else if(obj==ui->limiter)UpdateTooltipLabelUnit(obj,QString::number(roundf(translate(d,100,0,0,-30)*100)/100)+"dB (" + QString::number(d) + ")",alt==nullptr);
    //Headphone Engine
    else if(obj==ui->vhplvl)UpdateTooltipLabelUnit(obj,"Level " + QString::number(d+1),alt==nullptr);
    //Reverb
    else if(obj==ui->roomsize){
        long double x = d;
        UpdateTooltipLabelUnit(obj,QString::number(roundf(3.958333333* pow(10L,-7L) *pow(x,5L)- 4.106206294* pow(10L,-5L) *pow(x,4L)+ 1.189175408* pow(10L,-3L) *pow(x,3L)+ 4.16448133 * pow(10L,-3L) *pow(x,2L)+ 9.190238943 * pow(10L,-1L) * x+ 25.11013978))+"m\u00B2 (" + QString::number(d) + ")",alt==nullptr);
    }
    else if(obj==ui->roomwidth){
        long double x = d;
        long double in = -1.121794872 * pow(10L,-8L) * pow(x,5L) + 3.270687646 * pow(10L,-6L) * pow(x,4L) - 2.643502331 * pow(10L,-4L) * pow(x,3L) + 7.749854312 * pow(10L,-3L) * pow(x,2L) + 2.916958039 * pow(10L,-2L) * x+ 5.036713287;
        UpdateTooltipLabelUnit(obj,QString::number(roundf(in))+"m (" + QString::number(d) + ")",alt==nullptr);
    }
    //Compressor
    else if(obj==ui->comp_thres)  UpdateTooltipLabelUnit(obj,MathFunctions::buildCompressorUnitString(0,d),alt==nullptr);
    else if(obj==ui->comp_ratio)  UpdateTooltipLabelUnit(obj,MathFunctions::buildCompressorUnitString(1,d),alt==nullptr);
    else if(obj==ui->compwidth)   UpdateTooltipLabelUnit(obj,MathFunctions::buildCompressorUnitString(2,d),alt==nullptr);
    else if(obj==ui->compgain)    UpdateTooltipLabelUnit(obj,MathFunctions::buildCompressorUnitString(3,d),alt==nullptr);
    else if(obj==ui->compattack)  UpdateTooltipLabelUnit(obj,MathFunctions::buildCompressorUnitString(4,d),alt==nullptr);
    else if(obj==ui->comprelease) UpdateTooltipLabelUnit(obj,MathFunctions::buildCompressorUnitString(5,d),alt==nullptr);
    else if(obj==ui->a_kneewidth) UpdateTooltipLabelUnit(obj,MathFunctions::buildCompressorUnitString(6,d),alt==nullptr);
    else if(obj==ui->a_maxatk)    UpdateTooltipLabelUnit(obj,MathFunctions::buildCompressorUnitString(7,d),alt==nullptr);
    else if(obj==ui->a_maxrel)    UpdateTooltipLabelUnit(obj,MathFunctions::buildCompressorUnitString(8,d),alt==nullptr);
    else if(obj==ui->a_crest)     UpdateTooltipLabelUnit(obj,MathFunctions::buildCompressorUnitString(9,d),alt==nullptr);
    else if(obj==ui->a_adapt)     UpdateTooltipLabelUnit(obj,MathFunctions::buildCompressorUnitString(10,d),alt==nullptr);
    else{
        //Reverb
        if(obj==ui->roomdamp)post = "%";
        else if(obj==ui->wet)post = "%";
        else if(obj==ui->dry)post = "%";
        //Bass
        else if(obj==ui->vbfreq)post = "Hz";
        //Spectrum Expend
        else if(obj==ui->barkcon)pre = "Level ";
        else if(obj==ui->barkfreq)post = "Hz";
        //Convolver
        else if(obj==ui->convcc)post = "%";
        UpdateTooltipLabelUnit(obj,pre + QString::number(d) + post,alt==nullptr);
    }
    if(!lockapply||obj!=nullptr)OnUpdate(false);
}
void MainWindow::UpdateTooltipLabelUnit(QObject* sender,QString text,bool viasignal){
    QWidget *w = qobject_cast<QWidget*>(sender);
    w->setToolTip(text);
    if(viasignal)ui->info->setText(text);
}
void MainWindow::UpdateAllUnitLabels(){
    UpdateUnitLabel(ui->colmwide->value(),ui->colmwide);
    UpdateUnitLabel(ui->colmdepth->value(),ui->colmdepth);
    UpdateUnitLabel(ui->colmmidimg->value(),ui->colmmidimg);
    UpdateUnitLabel(ui->vcmode->value(),ui->vcmode);
    UpdateUnitLabel(ui->vclvl->value(),ui->vclvl);
    UpdateUnitLabel(ui->vbmode->value(),ui->vbmode);
    UpdateUnitLabel(ui->vbfreq->value(),ui->vbfreq);
    UpdateUnitLabel(ui->vbgain->value(),ui->vbgain);
    UpdateUnitLabel(ui->vhplvl->value(),ui->vhplvl);
    UpdateUnitLabel(ui->difflvl->value(),ui->difflvl);
    UpdateUnitLabel(ui->roomsize->value(),ui->roomsize);
    UpdateUnitLabel(ui->roomwidth->value(),ui->roomwidth);
    UpdateUnitLabel(ui->roomdamp->value(),ui->roomdamp);
    UpdateUnitLabel(ui->wet->value(),ui->wet);
    UpdateUnitLabel(ui->dry->value(),ui->dry);
    UpdateUnitLabel(ui->gain->value(),ui->gain);
    UpdateUnitLabel(ui->maxgain->value(),ui->maxgain);
    UpdateUnitLabel(ui->maxvol->value(),ui->maxvol);
    UpdateUnitLabel(ui->limiter->value(),ui->limiter);
    UpdateUnitLabel(ui->outputpan->value(),ui->outputpan);
    UpdateUnitLabel(ui->outvolume->value(),ui->outvolume);
    UpdateUnitLabel(ui->comp_thres->value(),ui->comp_thres);
    UpdateUnitLabel(ui->compgain->value(),ui->compgain);
    UpdateUnitLabel(ui->compwidth->value(),ui->compwidth);
    UpdateUnitLabel(ui->comp_ratio->value(),ui->comp_ratio);
    UpdateUnitLabel(ui->compattack->value(),ui->compattack);
    UpdateUnitLabel(ui->comprelease->value(),ui->comprelease);
    UpdateUnitLabel(ui->a_adapt->value(),ui->a_adapt);
    UpdateUnitLabel(ui->a_crest->value(),ui->a_crest);
    UpdateUnitLabel(ui->a_maxatk->value(),ui->a_maxatk);
    UpdateUnitLabel(ui->a_maxrel->value(),ui->a_maxrel);
    UpdateUnitLabel(ui->a_kneewidth->value(),ui->a_kneewidth);
    UpdateUnitLabel(ui->vcurelvl->value(),ui->vcurelvl);
    UpdateUnitLabel(ui->axmode->value(),ui->axmode);
    UpdateUnitLabel(ui->barkcon->value(),ui->barkcon);
    UpdateUnitLabel(ui->barkfreq->value(),ui->barkfreq);
    UpdateUnitLabel(ui->convcc->value(),ui->convcc);
    UpdateUnitLabel(ui->dyn_xcoeff1->value(),ui->dyn_xcoeff1);
    UpdateUnitLabel(ui->dyn_xcoeff2->value(),ui->dyn_xcoeff2);
    UpdateUnitLabel(ui->dyn_ycoeff1->value(),ui->dyn_ycoeff1);
    UpdateUnitLabel(ui->dyn_ycoeff2->value(),ui->dyn_ycoeff2);
    UpdateUnitLabel(ui->dyn_sidegain1->value(),ui->dyn_sidegain1);
    UpdateUnitLabel(ui->dyn_sidegain2->value(),ui->dyn_sidegain2);
    UpdateUnitLabel(ui->dyn_bassgain->value(),ui->dyn_bassgain);
}

//---Helper
void MainWindow::SetEQ(const int* data){
    lockapply=true;
    ui->eq1->setValue(data[0]);
    ui->eq2->setValue(data[1]);
    ui->eq3->setValue(data[2]);
    ui->eq4->setValue(data[3]);
    ui->eq5->setValue(data[4]);
    ui->eq6->setValue(data[5]);
    ui->eq7->setValue(data[6]);
    ui->eq8->setValue(data[7]);
    ui->eq9->setValue(data[8]);
    ui->eq10->setValue(data[9]);
    lockapply=false;
    OnUpdate(true);
}
void MainWindow::CopyEQ(){
    QString s = QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10").arg(ui->eq1->value()).arg(ui->eq2->value()).arg(ui->eq3->value())
                                                         .arg(ui->eq4->value()).arg(ui->eq5->value()).arg(ui->eq6->value())
                                                         .arg(ui->eq7->value()).arg(ui->eq8->value()).arg(ui->eq9->value())
                                                         .arg(ui->eq10->value());
    app->clipboard()->setText(s);
}
void MainWindow::PasteEQ(){
    QClipboard* a = app->clipboard();
    std::string str = a->text().toUtf8().constData();
    std::vector<int> vect;
    std::stringstream ss(str);
    int i;
    while (ss >> i)
    {
        vect.push_back(i);
        if (ss.peek() == ',')
            ss.ignore();
    }
    int data[100];
    std::copy(vect.begin(), vect.end(), data);
    SetEQ(data);
}
void MainWindow::ResetEQ(){
    SetEQ(EQ::defaultEQPreset());
}
void MainWindow::SetIRS(const string& irs,bool apply){
    activeirs = irs;
    QFileInfo irsInfo(QString::fromStdString(irs));
    ui->convpath->setText(irsInfo.baseName());
    ui->convpath->setCursorPosition(0);
    if(apply)ApplyConfig();
}
AppConfigWrapper* MainWindow::getACWrapper(){
    return m_appwrapper;
}

//---Connect UI-Signals
void MainWindow::ConnectActions(){
    connect(ui->apply, SIGNAL(clicked()), this, SLOT(ApplyConfig()));
    connect(ui->disableFX, SIGNAL(clicked()), this, SLOT(DisableFX()));
    connect(ui->reset_eq, SIGNAL(clicked()), this, SLOT(ResetEQ()));
    connect(ui->reset, SIGNAL(clicked()), this, SLOT(Reset()));
    connect(ui->conv_select, SIGNAL(clicked()), this, SLOT(DialogHandler()));
    connect(ui->copy_eq, SIGNAL(clicked()), this, SLOT(CopyEQ()));
    connect(ui->paste_eq, SIGNAL(clicked()), this, SLOT(PasteEQ()));

    connect(ui->cpreset, SIGNAL(clicked()), this, SLOT(DialogHandler()));
    connect(ui->set, SIGNAL(clicked()), this, SLOT(DialogHandler()));

    connect( ui->convcc , SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect( ui->vbfreq , SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect( ui->vbgain, SIGNAL(valueChanged(int)),this,  SLOT(UpdateUnitLabel(int)));
    connect( ui->vbmode , SIGNAL(valueChanged(int)),this,  SLOT(UpdateUnitLabel(int)));
    connect( ui->difflvl , SIGNAL(valueChanged(int)),this,  SLOT(UpdateUnitLabel(int)));
    connect( ui->vhplvl , SIGNAL(valueChanged(int)),this,  SLOT(UpdateUnitLabel(int)));
    connect( ui->roomsize , SIGNAL(valueChanged(int)),this,  SLOT(UpdateUnitLabel(int)));
    connect( ui->roomwidth , SIGNAL(valueChanged(int)),this,  SLOT(UpdateUnitLabel(int)));
    connect( ui->roomdamp , SIGNAL(valueChanged(int)),this,  SLOT(UpdateUnitLabel(int)));
    connect( ui->wet , SIGNAL(valueChanged(int)),this,  SLOT(UpdateUnitLabel(int)));
    connect( ui->dry , SIGNAL(valueChanged(int)),this,  SLOT(UpdateUnitLabel(int)));
    connect( ui->colmwide , SIGNAL(valueChanged(int)),this,  SLOT(UpdateUnitLabel(int)));
    connect( ui->colmmidimg , SIGNAL(valueChanged(int)),this,  SLOT(UpdateUnitLabel(int)));
    connect( ui->colmdepth, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect( ui->vclvl, SIGNAL(valueChanged(int)),this,  SLOT(UpdateUnitLabel(int)));
    connect( ui->vcmode, SIGNAL(valueChanged(int)),this,  SLOT(UpdateUnitLabel(int)));
    connect( ui->gain , SIGNAL(valueChanged(int)),this,  SLOT(UpdateUnitLabel(int)));
    connect( ui->maxgain , SIGNAL(valueChanged(int)),this,  SLOT(UpdateUnitLabel(int)));
    connect( ui->maxvol , SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect( ui->outputpan , SIGNAL(valueChanged(int)),this,  SLOT(UpdateUnitLabel(int)));
    connect( ui->limiter , SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect( ui->outvolume , SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->vcurelvl, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->axmode, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->barkfreq, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->barkcon, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->comprelease, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->compgain, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->compwidth, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->comp_ratio, SIGNAL(valueChanged(int)),this,SLOT(UpdateUnitLabel(int)));
    connect(ui->comp_thres, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->compattack, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->comprelease, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->a_adapt, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->a_crest, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->a_maxatk, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->a_maxrel, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->a_kneewidth, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));

    connect(ui->eq1, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->eq2, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->eq3, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->eq4, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->eq5, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->eq6, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->eq7, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->eq8, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->eq9, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->eq10, SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect(ui->eqpreset, SIGNAL(currentIndexChanged(int)),this,SLOT(EqPresetSelectionUpdated()));

    connect( ui->vb , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->clarity , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->vcure , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->tubesim , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->vhp , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->diff , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->reverb , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->enable_eq , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->enable_comp , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->noclip , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->m_gain , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->m_width , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->m_attack , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->m_release , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->vb , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->clarity , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->vcure , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->tubesim , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->agc , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->colm , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->vse , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->conv , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->ax , SIGNAL(clicked()),this, SLOT(OnUpdate()));

    connect(ui->dynsys_preset, SIGNAL(currentIndexChanged(int)),this,SLOT(DynsysPresetSelectionUpdated()));
    connect( ui->dynsys , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->dyn_xcoeff1 , SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect( ui->dyn_xcoeff2 , SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect( ui->dyn_ycoeff1 , SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect( ui->dyn_ycoeff2 , SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect( ui->dyn_bassgain , SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect( ui->dyn_sidegain1 , SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect( ui->dyn_sidegain2 , SIGNAL(valueChanged(int)),this, SLOT(UpdateUnitLabel(int)));
    connect( ui->convcc , SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect( ui->vbfreq , SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect( ui->vbgain, SIGNAL(sliderReleased()),this,  SLOT(OnRelease()));
    connect( ui->vbmode , SIGNAL(valueChanged(int)),this,  SLOT(OnRelease()));
    connect( ui->difflvl , SIGNAL(sliderReleased()),this,  SLOT(OnRelease()));
    connect( ui->vhplvl , SIGNAL(sliderReleased()),this,  SLOT(OnRelease()));
    connect( ui->roomsize , SIGNAL(sliderReleased()),this,  SLOT(OnRelease()));
    connect( ui->roomwidth , SIGNAL(sliderReleased()),this,  SLOT(OnRelease()));
    connect( ui->roomdamp , SIGNAL(sliderReleased()),this,  SLOT(OnRelease()));
    connect( ui->wet , SIGNAL(sliderReleased()),this,  SLOT(OnRelease()));
    connect( ui->dry , SIGNAL(sliderReleased()),this,  SLOT(OnRelease()));
    connect( ui->colmwide , SIGNAL(sliderReleased()),this,  SLOT(OnRelease()));
    connect( ui->colmmidimg , SIGNAL(sliderReleased()),this,  SLOT(OnRelease()));
    connect( ui->colmdepth, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect( ui->vclvl, SIGNAL(sliderReleased()),this,  SLOT(OnRelease()));
    connect( ui->vcmode, SIGNAL(valueChanged(int)),this,  SLOT(OnRelease()));
    connect( ui->gain , SIGNAL(sliderReleased()),this,  SLOT(OnRelease()));
    connect( ui->maxgain , SIGNAL(sliderReleased()),this,  SLOT(OnRelease()));
    connect( ui->maxvol , SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect( ui->outputpan , SIGNAL(sliderReleased()),this,  SLOT(OnRelease()));
    connect( ui->limiter , SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect( ui->outvolume , SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->vcurelvl, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->axmode, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->barkfreq, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->barkcon, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->comprelease, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->compgain, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->compwidth, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->comp_ratio, SIGNAL(sliderReleased()),this,SLOT(OnRelease()));
    connect(ui->comp_thres, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->compattack, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->comprelease, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->a_adapt, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->a_crest, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->a_maxatk, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->a_maxrel, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->a_kneewidth, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));

    connect( ui->dyn_xcoeff1 , SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect( ui->dyn_xcoeff2 , SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect( ui->dyn_ycoeff1 , SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect( ui->dyn_ycoeff2 , SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect( ui->dyn_bassgain , SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect( ui->dyn_sidegain1 , SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect( ui->dyn_sidegain2 , SIGNAL(sliderReleased()),this, SLOT(OnRelease()));

    connect(ui->eq1, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq2, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq3, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq4, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq5, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq6, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq7, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq8, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq9, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq10, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
}
