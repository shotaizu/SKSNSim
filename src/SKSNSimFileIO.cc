/**********************************
 * File: SKSNSimFileIO.cc
 **********************************/

#include <set>
#include <cstdio>
#include <fstream>
#include <TFile.h>
#include <TFileCacheWrite.h>
#include <TTree.h>
#include <mcinfo.h>
#include "SKSNSimFileIO.hh"
#include "skrun.h"

/*
 * PROTOTYPE DECLARARATION for internal use
 */
std::vector<std::tuple<int,int,int>> ReadTimeEventFile(TRandom &rndgen, int runBegin, int runEnd, double nev_per_min = 1.0);

void SKSNSimFileOutTFile::Open(const std::string fname) {
  if(m_fileptr != NULL){
    Close();
    delete m_fileptr;
  }

  m_fileptr = new TFile(fname.c_str(), "RECREATE");
  std::cout << "Opened : " << fname << " (" << m_fileptr << ") "<< std::endl;
	//------------------------------------------------------------------------
	// set write cache to 40MB 
	TFileCacheWrite *cachew = new TFileCacheWrite(m_fileptr, 40*1024*1024);
	//------------------------------------------------------------------------

	/*-----define class-----*/
	m_MC = new MCInfo;
	m_MC->Clear();
	m_MC->SetName("MC");

	/*-----define branch-----*/
	TList *TopBranch = new TList;
	TopBranch->Add(m_MC);

	// define tree
	m_OutTree = new TTree("data", "SK 5 tree");
	// new MF
	m_OutTree->SetCacheSize(40*1024*1024);
	int bufsize = 8*1024*1024;      // may be this is the best 15-OCT-2007 Y.T.
	m_OutTree->Branch(TopBranch,bufsize);
}

void SKSNSimFileOutTFile::Close(){
  std::cout << m_fileptr->GetName() << std::endl;
	m_fileptr->cd();
	m_OutTree->Write();
  m_fileptr->Save();
	m_OutTree->Reset();
	m_fileptr->Close();
}

void SKSNSimFileOutTFile::Write(const SKSNSimSNEventVector &ev){

  m_MC->mcinfo[0] = ev.GetRunnum();
  m_MC->mcinfo[1] = ev.GetSubRunnum();

  // MCVERTEX (see $SKOFL_ROOT/inc/vcvrtx.h )
  m_MC->nvtxvc = ev.GetNVertex();
  for(int i = 0; i < ev.GetNVertex(); i++){
    m_MC->pvtxvc[i][0] = ev.GetVertexPositionX(i);
    m_MC->pvtxvc[i][1] = ev.GetVertexPositionY(i);
    m_MC->pvtxvc[i][2] = ev.GetVertexPositionZ(i);
    m_MC->timvvc[i] = ev.GetVertexTime(i);
    m_MC->iflvvc[i] = ev.GetVertexIFLVVC(i);
    m_MC->iparvc[i] = ev.GetVertexIPARVC(i);
  }


  m_MC->nvc = ev.GetNTrack();
  for(int i = 0; i < ev.GetNTrack(); i++){
    m_MC->ipvc[i] = ev.GetTrackPID(i);
    m_MC->energy[i] = ev.GetTrackEnergy(i);
    m_MC->pvc[i][0] = ev.GetTrackMomentumX(i);
    m_MC->pvc[i][1] = ev.GetTrackMomentumY(i);
    m_MC->pvc[i][2] = ev.GetTrackMomentumZ(i);
    m_MC->iorgvc[i] = ev.GetTrackIORGVC(i);
    m_MC->ivtivc[i] = ev.GetTrackIVTIVC(i);
    m_MC->ivtfvc[i] = ev.GetTrackIVTFVC(i);
    m_MC->iflgvc[i] = ev.GetTrackIFLGVC(i);
    m_MC->icrnvc[i] = ev.GetTrackICRNVC(i);
  }

  m_OutTree->Fill();
}

std::vector<SKSNSimFileSet> GenerateOutputFileListNoRuntime(const SKSNSimUserConfiguration &conf){
  constexpr int MCRUN = 999999;
  std::vector<SKSNSimFileSet> flist;
  if( conf.GetNormRuntime() ) return flist;
  int nfile = conf.GetNumEvents() / conf.GetNumEventsPerFile();
  int nfile_rem = conf.GetNumEvents() % conf.GetNumEventsPerFile();



  auto genFileName = [](const SKSNSimUserConfiguration &c, int i){
    char buf[1000];
    snprintf(buf, 999, "%s/%s_%06d.root", c.GetOutputDirectory().c_str(), c.GetOutputPrefix().c_str(), i);
    return std::string(buf);
  };

  for(int i = 0; i < nfile; i++){
    auto fn = genFileName(conf, i);
    flist.push_back(SKSNSimFileSet(fn, MCRUN, -1, conf.GetNumEventsPerFile()));
  }
  if( nfile_rem != 0){
    auto fn = genFileName(conf, nfile);
    flist.push_back(SKSNSimFileSet(fn, MCRUN, -1, nfile_rem));
  }
  return flist;
}
std::vector<SKSNSimFileSet> GenerateOutputFileListRuntime(SKSNSimUserConfiguration &conf){
  std::vector<SKSNSimFileSet> buffer;
  if( !conf.GetNormRuntime() ) return buffer;

  const int runBegin = conf.GetRuntimeRunBegin();
  const int runEnd   = conf.GetRuntimeRunEnd();

  auto runev = ReadTimeEventFile(*conf.GetRandomGenerator(), runBegin, runEnd, conf.GetRuntimeFactor());

  auto genFileName = [](const SKSNSimUserConfiguration &c, int r,int sr){
    char buf[1000];
    snprintf(buf, 999, "%s/%s_r%06d_%06d.root", c.GetOutputDirectory().c_str(), c.GetOutputPrefix().c_str(), r, sr);
    return std::string(buf);
  };
  for(auto it = runev.begin(); it != runev.end(); it++){
    int run = std::get<0>(*it);
    int subrun = std::get<1>(*it);
    int nev = std::get<2>(*it);
    auto fn = genFileName(conf, run, subrun);
    buffer.push_back(SKSNSimFileSet(fn, run,subrun, nev));
  }
  
  return buffer;
}
std::vector<SKSNSimFileSet> GenerateOutputFileList(SKSNSimUserConfiguration &conf){
  std::vector<SKSNSimFileSet> flist;
  if( conf.GetNormRuntime() )
    flist = GenerateOutputFileListRuntime(conf);
  else
    flist = GenerateOutputFileListNoRuntime(conf);

  return flist;
}

SKRUN FindSKPeriod(int run){
    if ( run < SK_IV_BEGIN )
      return SK_I_II_III_BEGIN;
    else if ( SK_IV_BEGIN <= run && run <= SK_IV_END )
      return SK_IV_BEGIN;
    else if ( SK_V_BEGIN <= run && run <= SK_V_END ) 
      return SK_V_BEGIN;
    else if ( SK_VI_BEGIN <= run && run <= SK_VI_END)
      return SK_VI_BEGIN;
    else if (SK_VII_BEGIN <= run )
      return SK_VII_BEGIN;
    
    return SK_I_II_III_BEGIN;
}

std::string FindTimeFile(int run) {
    if ( run < SK_IV_BEGIN || run >= SK_VII_BEGIN ) {
      std::cout << "reference run number is not correct"<<std::endl;
      return "";
    } 
    else if ( SK_IV_BEGIN <= run && run < SK_IV_END ) { 
      return "/home/sklowe/realtime_sk4_rep/solar_apr19/timevent/livesubruns.r061525.r077958"; 
    }
    else if ( SK_V_BEGIN <= run && run < SK_V_END ) { 
      return "/home/sklowe/realtime_sk5_rep/solar_nov20/timevent/livesubruns.r080539.r082915";
    }
    else if ( SK_VI_BEGIN <= run && run < SK_VI_END ) { 
      return "/home/sklowe/realtime_sk6_rep/solar_may22/timevent/livesubruns.r085000.r087073"; 
    }
    return "";
}

void LoadTimeEventsFromFile(std::vector<std::tuple<int,int,double>> &buffer, SKRUN p){
  const std::string fname = FindTimeFile(p);
  buffer.clear();
  if(fname=="") return;

  std::ifstream ifs(fname);
  if(!ifs.is_open()){
    std::cerr << "Failed to open : " << fname << std::endl;
    return;
  }

  std::cout << "File opend: " << fname << std::endl;
  for(std::array<char,500> b; ifs.getline(&b[0], 500); ){
    int r, sr;
    double t, tt;
    sscanf(&b[0], "%d %d %lf %lf", &r, &sr, &t, &tt);
    buffer.push_back( std::make_tuple(r,sr,t));
  }

  ifs.close();
  return;
}

std::vector<std::tuple<int,int,int>> ReadTimeEventFile(TRandom &rndgen, int runBegin, int runEnd, double nev_per_min)
{
  std::cout <<" Estimate # of event from timevent file "<<std::endl;
  std::vector<std::tuple<int,int,int>> runev; // run,subrun,nev
  std::vector<std::tuple<int,int,double>> runtime; // run,subrun,time

  auto skperiodBegin = FindSKPeriod(runBegin);
  auto skperiodEnd   = FindSKPeriod(runEnd);
  std::cout << "Period run = [" << skperiodBegin << ", " << skperiodEnd << ")" << std::endl;
  std::set<SKRUN> skperiods{SK_IV_BEGIN, SK_V_BEGIN, SK_VI_BEGIN, SK_VII_BEGIN};
  for(auto it = skperiods.begin(); it != skperiods.end();){
    if(skperiodBegin <= *it && *it <= skperiodEnd ) it++;
    else 
      it = skperiods.erase(it);
  }
  
  for(auto it = skperiods.begin(); it != skperiods.end(); it++)
    LoadTimeEventsFromFile( runtime, *it);

  for(auto it = runtime.begin(); it != runtime.end(); ){
    if( runBegin <= std::get<0>(*it) && std::get<0>(*it) < runEnd ) it++;
    else it = runtime.erase(it);
  }

  auto convDoubleToInt = [] (TRandom &r, double d, double w){
    double nev_double = d * w;
    int nev = int(nev_double);
    double nev_frac = nev_double - nev; 
    double rand = r.Rndm();
    int nev_frac_int = 0;
    if(nev_frac < rand) nev_frac_int = 0;
    else nev_frac_int = 1;
    return nev+nev_frac_int;
  };

  const double weight = nev_per_min / 60.0;
  for(auto it = runtime.begin(); it != runtime.end(); it++)
    runev.push_back(
        std::make_tuple( std::get<0>(*it), std::get<1>(*it),
          convDoubleToInt(rndgen, std::get<2>(*it), weight)));

  return runev;
}
