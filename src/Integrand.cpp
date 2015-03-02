#include "interface/Integrand.h"



MEM::Integrand::Integrand(MEM::Hypothesis hyp, int debug){
  hypo               = hyp;
  debug_code         = debug;
  num_of_vars        = 1;
  naive_jet_counting = 0;
  fs                 = FinalState::Undefined;
  ig2                = nullptr;

  if( debug_code&DebugVerbosity::init )  
    cout << "Integrand::Integrand(): START" << endl;
}

MEM::Integrand::~Integrand(){
  if( debug_code&DebugVerbosity::init )  
    cout << "Integrand::~Integrand()" << endl;    
  clear();
}


/* 
   Initialise parameters (***once per event***)
   - determine final state 
   - save jet informations 
   - create list of permutations
   - determine number of variables
*/
void MEM::Integrand::init(){

  if( debug_code&DebugVerbosity::init ){
    cout << "Integrand::init(): START" << endl;
  }    

  // deal with leptons :
  // decide the final state based on the lepton multiplicity
  switch( obs_leptons.size() ){
  case 0:
    fs = FinalState::HH;
    break;
  case 1:
    fs = FinalState::LH;
    break;
  case 2:
    fs = FinalState::LL;
    break;
  default:
    cout << "*** MEM::Intgrator::init(): Unsupported final state" << endl;
    break;
  }

  // a class member: used to keep track of how many quarks are expected
  naive_jet_counting = 8*(fs==FinalState::HH) + 6*(fs==FinalState::LH) + 4*(fs==FinalState::LL);

  // deal with jets:
  // if less jets are recorded than the naive_jet_counting,
  // fill in perm_index with -1;  
  size_t n_jets = obs_jets.size();
  vector<int> perm_index;
  for(size_t id = 0; id < n_jets ; ++id) 
    perm_index.push_back( id );

  while( perm_index.size() < naive_jet_counting)
    perm_index.push_back( -1 );

  // calculate upper / lower edges
  for( auto j : obs_jets ){
    double y[2] = { j->p4().E(), j->p4().Eta() };
    pair<double, double> edges;
    edges = get_support( y, TFType::qReco, 0.98 ) ;
    j->addObs( Observable::E_LOW_Q,  edges.first  );
    j->addObs( Observable::E_HIGH_Q, edges.second );
    edges = get_support( y, TFType::bReco, 0.98 ) ;
    j->addObs( Observable::E_LOW_B,  edges.first  );
    j->addObs( Observable::E_HIGH_B, edges.second );    
  }
  
  if( debug_code&DebugVerbosity::init ){
    cout << "\tIndexes to be permuted: [ " ;
    for( auto ind : perm_index ) cout << ind << " ";
    cout << "]" << endl;
  }

  CompPerm comparator;
  sort( perm_index.begin(), perm_index.end(), comparator);
  size_t n_perm{0};
  do{
    perm_indexes.push_back( perm_index );
    if( debug_code&DebugVerbosity::init&0 ) {
      cout << "\tperm. " << n_perm << ": [ ";
      for( auto ind : perm_index ) cout << ind << " ";
      cout << "]" << endl;
    }
    ++n_perm;
  } while( next_permutation( perm_index.begin(), perm_index.end(), comparator ) );
  if( debug_code&DebugVerbosity::init ){
    cout << "\tTotal of " << n_perm << " permutation(s) created" << endl;
  }
  
  // Formula to get the number of unknowns
  // The number of variables is equal to npar - 2*extra_jets
  num_of_vars = 
    24                                   // dimension of the phase-space
    -3*obs_leptons.size()                // leptons
    -2*( TMath::Min(obs_jets.size(), naive_jet_counting) ) // jet directions
    -4                                   // top/W mass
    -1*(hypo==Hypothesis::TTH);          // H mass
  if( debug_code&DebugVerbosity::init ){
    cout << "\tTotal of " << num_of_vars << " unknowns" << endl;
  }

  if( debug_code&DebugVerbosity::init ){
    cout << "Integrand::init(): END" << endl;
  }

  return;
}

void MEM::Integrand::get_edges(double* lim, const initializer_list<PSVar>& lost, const size_t& nvar, const size_t& edge){

  // convention is: 
  //   even <=> cosTheta [-1,   +1]
  //   odd  <=> phi      [-PI, +PI]
  size_t count_extra{0};

  switch( fs ){
  case FinalState::LH:
    lim[map_to_var[PSVar::E_q1]]      =  edge ?  1. :  0.;
    lim[map_to_var[PSVar::cos_qbar2]] =  edge ? +1  : -1.;
    lim[map_to_var[PSVar::phi_qbar2]] =  edge ? +TMath::Pi() : -TMath::Pi();
    lim[map_to_var[PSVar::E_b]]       =  edge ?  1. :  0.;
    if( hypo==Hypothesis::TTBB ) map_to_var[PSVar::E_bbar] =  edge ?  1. :  0.;
    for( auto l = lost.begin() ; l !=lost.end(); ++l ){
      if(edge)
	lim[map_to_var[*l]] = count_extra%2==0 ? +1 : +TMath::Pi(); 
      else
	lim[map_to_var[*l]] = count_extra%2==0 ? -1 : -TMath::Pi(); 
      ++count_extra;
    }          
    break;
  case FinalState::LL:
    lim[map_to_var[PSVar::cos_qbar1]] =  edge ? +1  : -1.;
    lim[map_to_var[PSVar::phi_qbar1]] =  edge ? +TMath::Pi() : -TMath::Pi();
    lim[map_to_var[PSVar::cos_qbar2]] =  edge ? +1  : -1.;
    lim[map_to_var[PSVar::phi_qbar2]] =  edge ? +TMath::Pi() : -TMath::Pi();
    lim[map_to_var[PSVar::E_b]]       =  edge ?  1. :  0.;
    if( hypo==Hypothesis::TTBB ) map_to_var[PSVar::E_bbar] =  edge ?  1. :  0.;
    for( auto l = lost.begin() ; l !=lost.end(); ++l ){
      if(edge)
	lim[map_to_var[*l]] = count_extra%2==0 ? +1 : +TMath::Pi(); 
      else
	lim[map_to_var[*l]] = count_extra%2==0 ? -1 : -TMath::Pi(); 
      ++count_extra;
    }          
    break;
  case FinalState::HH:
  default:
    break;
  }

  if( debug_code&DebugVerbosity::init ){
    cout << "\tIntegrand::get_edges(): SUMMARY" << endl;
    cout << (edge ? "\t\tH" : "\t\tL") << " edges: [ ";
    for(size_t i = 0 ; i < nvar ; ++i) cout << lim[i] << " " ;
    cout << "]" << endl;
  }
 
}

double MEM::Integrand::get_width(const double* xL, const double* xU, const size_t nvar){
  double out{1.};
  for(size_t i = 0; i < nvar ; ++i) out *= TMath::Abs( xU[i]-xL[i] );
  return out;
}

void MEM::Integrand::fill_map(const initializer_list<PSVar>& lost){
  
  size_t count_extra{0};
  switch( fs ){
  case FinalState::LH:
    map_to_var[PSVar::E_q1]         = 0; //Eq 
    map_to_var[PSVar::cos_qbar2]    = 1; //cosNu
    map_to_var[PSVar::phi_qbar2]    = 2; //phiNu
    map_to_var[PSVar::E_b]          = 3; //Eb
    if( hypo==Hypothesis::TTBB ) map_to_var[PSVar::E_bbar] = 4;
    count_extra = 4+(hypo==Hypothesis::TTBB);
    for( auto l = lost.begin() ; l!=lost.end() ; ++l ) map_to_var[*l] = count_extra++;
    break;
  case FinalState::LL:
    map_to_var[PSVar::cos_qbar1]    = 0; //cosNu
    map_to_var[PSVar::phi_qbar1]    = 1; //phiNu
    map_to_var[PSVar::cos_qbar2]    = 2; //cosNu
    map_to_var[PSVar::phi_qbar2]    = 3; //phiNu
    map_to_var[PSVar::E_b]          = 4; //Eb
    if( hypo==Hypothesis::TTBB ) map_to_var[PSVar::E_bbar] = 5;
    count_extra = 5+(hypo==Hypothesis::TTBB);
    for( auto l = lost.begin() ; l!=lost.end() ; ++l ) map_to_var[*l] = count_extra++;
    break;
  case FinalState::HH:
  default:
    break;
  }

  if( debug_code&DebugVerbosity::init ){
    cout << "\tIntegrand::fill_map(): SUMMARY" << endl;
    for( auto iter = map_to_var.begin() ; iter!=map_to_var.end() ; ++iter ){
      cout << "\t\tPS[" << static_cast<int>(iter->first) << "] maps to x[" << iter->second << "]" << endl;
    }
  }

  return;
}

void MEM::Integrand::push_back_object(const LV& p4,  const MEM::ObjectType& type){

  Object* obj = new Object(p4, type);

  switch( type ){
  case ObjectType::Jet:
    obs_jets.push_back( obj ); 
    break;
  case ObjectType::Lepton:
    obs_leptons.push_back( obj ); 
    break;
  case ObjectType::MET:
  case ObjectType::Recoil:
    obs_mets.push_back( obj ); 
    break;
  default:
    cout << "*** MEM::Intgrator::push_back_object(): Unknown type of object added" << endl;
    break;
  }

  if( debug_code&DebugVerbosity::input ){
    cout << "Integrand::fill_map(): SUMMARY" << endl;
    obj->print(cout);
  }
  
  return;
}

void MEM::Integrand::run(){
  
  double prob{0.};

  if( debug_code&DebugVerbosity::init ){
    cout << "Integrand::run(): START" << endl;
  }

  init();

  ig2 = new ROOT::Math::GSLMCIntegrator(ROOT::Math::IntegrationMultiDim::kVEGAS, 1.e-12, 1.e-5, 10);

  prob += make_assumption( initializer_list<PSVar>{}  );
  //prob += make_assumption( initializer_list<PSVar>{PSVar::cos_qbar1, PSVar::phi_qbar1} );
  //prob += make_assumption( initializer_list<PSVar>{PSVar::cos_b1, PSVar::phi_b1} );
  //prob += make_assumption( initializer_list<PSVar>{PSVar::cos_qbar1, PSVar::phi_qbar1,PSVar::cos_b1, PSVar::phi_b1} );
  
  cout << "\tProbability = " << prob << endl;


  if( debug_code&DebugVerbosity::init ){
    cout << "Integrand::run(): END" << endl;
  }
  delete ig2;

  clear();

  return;
}

void MEM::Integrand::clear(){
  if( debug_code&DebugVerbosity::init ){
    cout << "Integrand::clear(): START" << endl;
  }
  for( auto j : obs_jets )     delete j;
  for( auto l : obs_leptons )  delete l;
  for( auto m : obs_mets )     delete m;
  obs_jets.clear();
  obs_leptons.clear();
  obs_mets.clear();
  if( debug_code&DebugVerbosity::init ){
    cout << "Integrand::clear(): END" << endl;
  }
}

bool MEM::Integrand::test_assumption( const size_t& lost, size_t& extra_jets){

  if( (obs_jets.size()+lost) < naive_jet_counting){
    if( debug_code&DebugVerbosity::init ) cout << "\t This assumption cannot be made: too few jets" << endl;
    return false;
  }
  extra_jets = (obs_jets.size()+lost-naive_jet_counting);
  return true;
}

double MEM::Integrand::make_assumption( initializer_list<MEM::PSVar>&& lost){

  if( debug_code&DebugVerbosity::init ){
    cout << "Integrand::make_assumption(): START" << endl;
  }

  double prob{0.};
  size_t extra_jets{0};
  if(!test_assumption(lost.size()/2, extra_jets)) return prob;

  perm_indexes_assumption.clear();

  // extra variables to integrate over  
  fill_map( lost );  

  // remove unwanted permutations
  for( auto perm : perm_indexes ){    
    auto good_perm = perm;
    size_t count{0};
    for(auto it = lost.begin() ; it!=lost.end() ; ++count, ++it ){
      if(count%2==0) perm[ (static_cast<size_t>(*it)-1)/3 ] = -1;      
    }
    count = 0;
    for(auto ind : perm){if(ind<0) ++count;}
    if(count==(lost.size()/2))
      perm_indexes_assumption.push_back( perm );    
  }  

  if( debug_code&DebugVerbosity::init ) {
    cout << "\tAssumption: Total of " << perm_indexes_assumption.size() << " considered" << endl;
    size_t n_perm{0};
    for( auto perm : perm_indexes_assumption ){
      cout << "\tperm. " << n_perm++ << ": [ ";
      for( auto ind : perm )
	cout << ind << " ";
      cout << "]" << endl;
    }
  }


  // create integration ranges
  size_t npar = num_of_vars+2*extra_jets;

  double xL[npar], xU[npar];
  get_edges(xL, lost, npar, 0);
  get_edges(xU, lost, npar, 1);      
  
  // function
  ROOT::Math::Functor toIntegrate(this, &MEM::Integrand::Eval, npar);  
  ig2->SetFunction(toIntegrate);
  
  // do the integral
  prob = ig2->Integral(xL,xU)/get_width(xL,xU,npar) ;

  if( debug_code&DebugVerbosity::init ){
    cout << "Integrand::make_assumption(): END" << endl;
  }
  
  return prob;
}

double MEM::Integrand::Eval(const double* x) const{
  double prob{0.};

  size_t n_perm{0};
  for( auto perm : perm_indexes_assumption ){
    if(n_perm>0) break;
    prob += probability(x, perm );
    ++n_perm;
  }

  return prob;
}

void MEM::Integrand::create_PS(MEM::PS& ps, const double* x, const vector<int>& perm ) const {

  if( debug_code&DebugVerbosity::integration ){
    cout << "\tIntegrand::create_PS(): START" << endl;
  }

  switch( fs ){
  case FinalState::LH:
    return create_PS_LH(ps, x, perm);
    break;
  case FinalState::LL:
    return create_PS_LL(ps, x, perm);
    break;
  case FinalState::HH:
    return create_PS_HH(ps, x, perm);
    break;
  default:
    break;
  }

  if( debug_code&DebugVerbosity::integration ){
    cout << "\tIntegrand::create_PS(): END" << endl;
  }
  
}


void MEM::Integrand::create_PS_LH(MEM::PS& ps, const double* x, const vector<int>& perm ) const {

  if( debug_code&DebugVerbosity::integration ){
    cout << "\tIntegrand::create_PS_LH(): START" << endl;
  }

  // jet,lepton counters
  size_t nj{0};
  size_t nl{0};

  // store temporary values to build four-vectors
  double E{0.};
  double E_LOW{0.};
  double E_HIGH{0.};
  double E_REC =  numeric_limits<double>::max();
  TVector3 dir(1.,0.,0.);

  //  PSVar::cos_q1, PSVar::phi_q1, PSVar::E_q1
  if( perm[nj]>=0 ){
    dir     = obs_jets[ perm[nj] ]->p4().Vect().Unit();
    E_LOW   = obs_jets[ perm[nj] ]->getObs(Observable::E_LOW_Q) ;
    E_HIGH  = obs_jets[ perm[nj] ]->getObs(Observable::E_HIGH_Q);
  }
  else{
    dir.SetTheta( TMath::ACos( x[ map_to_var.find(PSVar::cos_q1)->second ]) );
    dir.SetPhi  ( x[ map_to_var.find(PSVar::phi_q1)->second ] );
    E_LOW   = 0.;
    E_HIGH  = 1000.;
  }
  E       = E_LOW + (E_HIGH-E_LOW)*(x[ map_to_var.find(PSVar::E_q1)->second ]);
  extend_PS( ps, PSPart::q1, E, 0., dir, perm[nj], PSVar::cos_q1, PSVar::phi_q1, PSVar::E_q1, (perm[nj]>=0?TFType::qReco:TFType::qLost) ); 
  ++nj;

  //  PSVar::cos_qbar1, PSVar::phi_qbar1, PSVar::E_qbar1
  if( perm[nj]>=0 ){
    dir     = obs_jets[ perm[nj] ]->p4().Vect().Unit();
  }
  else{
    dir.SetTheta( TMath::ACos( x[ map_to_var.find(PSVar::cos_qbar1)->second ]) );
    dir.SetPhi  ( x[ map_to_var.find(PSVar::phi_qbar1)->second ] );
  }
  E    = solve( ps.lv(PSPart::q1), DMW2 , MQ, dir, E_REC );
  extend_PS( ps, PSPart::qbar1, E, 0., dir, perm[nj], PSVar::cos_qbar1, PSVar::phi_qbar1, PSVar::E_qbar1,  (perm[nj]>=0?TFType::qReco:TFType::qLost) ); 
  ++nj;

  //  PSVar::cos_b1, PSVar::phi_b1, PSVar::E_b1
  if( perm[nj]>=0 ){
    dir     = obs_jets[ perm[nj] ]->p4().Vect().Unit();
    E_REC   = obs_jets[ perm[nj] ]->p4().E();
  }
  else{
    dir.SetTheta( TMath::ACos( x[ map_to_var.find(PSVar::cos_b1)->second ]) );
    dir.SetPhi  ( x[ map_to_var.find(PSVar::phi_b1)->second ] );
  }
  E       = solve(ps.lv(PSPart::q1) + ps.lv(PSPart::qbar1), DMT2, MB, dir, E_REC);
  extend_PS( ps, PSPart::b1, E, MB , dir, perm[nj], PSVar::cos_b1, PSVar::phi_b1, PSVar::E_b1,  (perm[nj]>=0?TFType::bReco:TFType::bLost) ); 
  ++nj;
  
  //  PSVar::cos_q2, PSVar::phi_q2, PSVar::E_q2
  dir     = obs_leptons[ nl ]->p4().Vect().Unit(); 
  E       = obs_leptons[ nl ]->p4().E();
  extend_PS( ps, PSPart::q2, E, 0., dir, nl, PSVar::cos_q2, PSVar::phi_q2, PSVar::E_q2, TFType::muReco ); 
  ++nl;
    
  //  PSVar::cos_qbar2, PSVar::phi_qbar2, PSVar::E_qbar2
  dir.SetTheta( TMath::ACos( x[ map_to_var.find(PSVar::cos_qbar2)->second ]) );
  dir.SetPhi  ( x[ map_to_var.find(PSVar::phi_qbar2)->second ] );
  E    = solve( ps.lv(PSPart::q2), DMW2, ML, dir, E_REC );
  extend_PS( ps, PSPart::qbar2, E, 0., dir, -1, PSVar::cos_qbar2, PSVar::phi_qbar2, PSVar::E_qbar2, TFType::MET  ); 

  //  PSVar::cos_b2, PSVar::phi_b2, PSVar::E_b2   
  if( perm[nj]>=0 ){
    dir     =  obs_jets[ perm[nj] ]->p4().Vect().Unit();
    E_REC   = obs_jets[ perm[nj] ]->p4().E();
  }
  else{
    dir.SetTheta( TMath::ACos( x[ map_to_var.find(PSVar::cos_b2)->second ]) );
    dir.SetPhi  ( x[ map_to_var.find(PSVar::phi_b2)->second ] );
  }
  E       = solve( ps.lv(PSPart::q2) + ps.lv(PSPart::qbar2), DMT2, MB, dir, E_REC );
  extend_PS( ps, PSPart::b2, E, MB, dir, perm[nj], PSVar::cos_b2, PSVar::phi_b2, PSVar::E_b2,  (perm[nj]>=0?TFType::bReco:TFType::bLost) ); 
  ++nj;
  
  //  PSVar::cos_b, PSVar::phi_b, PSVar::E_b
  if( perm[nj]>=0 ){
    dir     = obs_jets[ perm[nj] ]->p4().Vect().Unit();
    E_LOW   = obs_jets[ perm[nj] ]->getObs(Observable::E_LOW_B) ;
    E_HIGH  = obs_jets[ perm[nj] ]->getObs(Observable::E_HIGH_B);
  }
  else{
    dir.SetTheta( TMath::ACos( x[ map_to_var.find(PSVar::cos_b)->second ]) );
    dir.SetPhi  ( x[ map_to_var.find(PSVar::phi_b)->second ] );
    E_LOW   = 0.;
    E_HIGH  = 1000.;
  }
  E       = E_LOW + (E_HIGH-E_LOW)*(x[ map_to_var.find(PSVar::E_b)->second ]);
  extend_PS( ps, PSPart::b, E, MB, dir, perm[nj], PSVar::cos_b, PSVar::phi_b, PSVar::E_b,  (perm[nj]>=0?TFType::bReco:TFType::bLost) ); 
  ++nj;

  //  PSVar::cos_bbar, PSVar::phi_bbar, PSVar::E_bbar
  if( perm[nj]>=0 ){
    dir     = obs_jets[ perm[nj] ]->p4().Vect().Unit();
    E_REC   = obs_jets[ perm[nj] ]->p4().E();
  }
  else{
    dir.SetTheta( TMath::ACos( x[ map_to_var.find(PSVar::cos_bbar)->second ]) );
    dir.SetPhi  ( x[ map_to_var.find(PSVar::phi_bbar)->second ] );
  }
  E    = hypo==Hypothesis::TTBB ? x[ map_to_var.find(PSVar::E_bbar)->second ]   : solve( ps.lv(PSPart::b), DMH2, MB, dir, E_REC);
  extend_PS( ps, PSPart::bbar, E, MB, dir, perm[nj], PSVar::cos_bbar, PSVar::phi_bbar, PSVar::E_bbar,  (perm[nj]>=0?TFType::bReco:TFType::bLost) ); 
  ++nj;

  if( debug_code&DebugVerbosity::integration ){
    cout << "\tIntegrand::create_PS_LH(): END" << endl;
  }

}

void MEM::Integrand::extend_PS(MEM::PS& ps, const MEM::PSPart& part, 
			       const double& E,  const double& M, const TVector3& dir,
			       const int& pos,  const PSVar& var_cos, const PSVar& var_phi, const PSVar& var_E, 
			       const TFType& type) const {

  double P       = TMath::Max( sqrt(E*E - M*M), M);
  ps.set(part,  MEM::GenPart(TLorentzVector( dir*P, E ), type ) );

  if( debug_code&DebugVerbosity::integration ){
    cout << "\t\tExtend phase-space point: adding variable " << static_cast<size_t>(part) << endl;
    if(  map_to_var.find(var_E)!=map_to_var.end() )
      cout << "\t\tE   = x[" <<  map_to_var.find(var_E)->second << "]" << endl;
    else
      cout << "\t\tE   = SOLVE" << endl;
    if(   map_to_var.find(var_cos)!= map_to_var.end() && map_to_var.find(var_phi)!= map_to_var.end()  ){
      cout << "\t\tcos = x[" << map_to_var.find(var_cos)->second << "]" << endl;    
      cout << "\t\tphi = x[" << map_to_var.find(var_phi)->second << "]" << endl;
    }
    else{
      cout << "\t\tUsing obs[" << pos << "]" << endl;
    }
  }
}


void MEM::Integrand::create_PS_LL(MEM::PS& ps, const double* x, const vector<int>& perm ) const {

  if( debug_code&DebugVerbosity::integration ){
    cout << "\tIntegrand::create_PS_LL(): START" << endl;
  }

  // jet,lepton counters
  size_t nj{0};
  size_t nl{0};

  // store temporary values to build four-vectors
  double E{0.};
  double E_LOW{0.};
  double E_HIGH{0.};
  double E_REC =  numeric_limits<double>::max();
  TVector3 dir(1.,0.,0.);

  //  PSVar::cos_q1, PSVar::phi_q1, PSVar::E_q1
  dir     = obs_leptons[ nl ]->p4().Vect().Unit(); 
  E       = obs_leptons[ nl ]->p4().E();
  extend_PS( ps, PSPart::q1, E, 0., dir, nl, PSVar::cos_q1, PSVar::phi_q1, PSVar::E_q1, TFType::muReco ); 
  ++nl;

  //  PSVar::cos_qbar2, PSVar::phi_qbar2, PSVar::E_qbar2
  dir.SetTheta( TMath::ACos( x[ map_to_var.find(PSVar::cos_qbar1)->second ]) );
  dir.SetPhi  ( x[ map_to_var.find(PSVar::phi_qbar1)->second ] );
  E    = solve( ps.lv(PSPart::q1), DMW2, ML, dir, E_REC );
  extend_PS( ps, PSPart::qbar1, E, 0., dir, -1, PSVar::cos_qbar1, PSVar::phi_qbar1, PSVar::E_qbar1, TFType::MET  ); 

  //  PSVar::cos_b1, PSVar::phi_b1, PSVar::E_b1
  if( perm[nj]>=0 ){
    dir     = obs_jets[ perm[nj] ]->p4().Vect().Unit();
    E_REC   = obs_jets[ perm[nj] ]->p4().E();
  }
  else{
    dir.SetTheta( TMath::ACos( x[ map_to_var.find(PSVar::cos_b1)->second ]) );
    dir.SetPhi  ( x[ map_to_var.find(PSVar::phi_b1)->second ] );
  }
  E       = solve(ps.lv(PSPart::q1) + ps.lv(PSPart::qbar1), DMT2, MB, dir, E_REC);
  extend_PS( ps, PSPart::b1, E, MB , dir, perm[nj], PSVar::cos_b1, PSVar::phi_b1, PSVar::E_b1,  (perm[nj]>=0?TFType::bReco:TFType::bLost) ); 
  ++nj;
  
  //  PSVar::cos_q2, PSVar::phi_q2, PSVar::E_q2
  dir     = obs_leptons[ nl ]->p4().Vect().Unit(); 
  E       = obs_leptons[ nl ]->p4().E();
  extend_PS( ps, PSPart::q2, E, 0., dir, nl, PSVar::cos_q2, PSVar::phi_q2, PSVar::E_q2, TFType::muReco ); 
  ++nl;
    
  //  PSVar::cos_qbar2, PSVar::phi_qbar2, PSVar::E_qbar2
  dir.SetTheta( TMath::ACos( x[ map_to_var.find(PSVar::cos_qbar2)->second ]) );
  dir.SetPhi  ( x[ map_to_var.find(PSVar::phi_qbar2)->second ] );
  E    = solve( ps.lv(PSPart::q2), DMW2, ML, dir, E_REC );
  extend_PS( ps, PSPart::qbar2, E, 0., dir, -1, PSVar::cos_qbar2, PSVar::phi_qbar2, PSVar::E_qbar2, TFType::MET  ); 

  //  PSVar::cos_b2, PSVar::phi_b2, PSVar::E_b2   
  if( perm[nj]>=0 ){
    dir     =  obs_jets[ perm[nj] ]->p4().Vect().Unit();
    E_REC   = obs_jets[ perm[nj] ]->p4().E();
  }
  else{
    dir.SetTheta( TMath::ACos( x[ map_to_var.find(PSVar::cos_b2)->second ]) );
    dir.SetPhi  ( x[ map_to_var.find(PSVar::phi_b2)->second ] );
  }
  E       = solve( ps.lv(PSPart::q2) + ps.lv(PSPart::qbar2), DMT2, MB, dir, E_REC );
  extend_PS( ps, PSPart::b2, E, MB, dir, perm[nj], PSVar::cos_b2, PSVar::phi_b2, PSVar::E_b2,  (perm[nj]>=0?TFType::bReco:TFType::bLost) ); 
  ++nj;
  
  //  PSVar::cos_b, PSVar::phi_b, PSVar::E_b
  if( perm[nj]>=0 ){
    dir     = obs_jets[ perm[nj] ]->p4().Vect().Unit();
    E_LOW   = obs_jets[ perm[nj] ]->getObs(Observable::E_LOW_B) ;
    E_HIGH  = obs_jets[ perm[nj] ]->getObs(Observable::E_HIGH_B);
  }
  else{
    dir.SetTheta( TMath::ACos( x[ map_to_var.find(PSVar::cos_b)->second ]) );
    dir.SetPhi  ( x[ map_to_var.find(PSVar::phi_b)->second ] );
    E_LOW   = 0.;
    E_HIGH  = 1000.;
  }
  E       = E_LOW + (E_HIGH-E_LOW)*(x[ map_to_var.find(PSVar::E_b)->second ]);
  extend_PS( ps, PSPart::b, E, MB, dir, perm[nj], PSVar::cos_b, PSVar::phi_b, PSVar::E_b,  (perm[nj]>=0?TFType::bReco:TFType::bLost) ); 
  ++nj;

  //  PSVar::cos_bbar, PSVar::phi_bbar, PSVar::E_bbar
  if( perm[nj]>=0 ){
    dir     = obs_jets[ perm[nj] ]->p4().Vect().Unit();
    E_REC   = obs_jets[ perm[nj] ]->p4().E();
  }
  else{
    dir.SetTheta( TMath::ACos( x[ map_to_var.find(PSVar::cos_bbar)->second ]) );
    dir.SetPhi  ( x[ map_to_var.find(PSVar::phi_bbar)->second ] );
  }
  E    = hypo==Hypothesis::TTBB ? x[ map_to_var.find(PSVar::E_bbar)->second ]   : solve( ps.lv(PSPart::b), DMH2, MB, dir, E_REC);
  extend_PS( ps, PSPart::bbar, E, MB, dir, perm[nj], PSVar::cos_bbar, PSVar::phi_bbar, PSVar::E_bbar,  (perm[nj]>=0?TFType::bReco:TFType::bLost) ); 
  ++nj;

  if( debug_code&DebugVerbosity::integration ){
    cout << "\tIntegrand::create_PS_LH(): END" << endl;
  }

}

void MEM::Integrand::create_PS_HH(MEM::PS& ps, const double* x, const vector<int>& perm ) const {}

double MEM::Integrand::probability(const double* x, const vector<int>& perm ) const {

  if( debug_code&DebugVerbosity::integration ){
    cout << "\tIntegrand::probability(): START" << endl;
  }

  PS ps;
  create_PS(ps, x, perm);
  if( debug_code&DebugVerbosity::integration ) ps.print(cout);

  double m   {1.}; // matrix element
  double w   {1.}; // transfer function
  double nu_x{0.}; // total nu's px
  double nu_y{0.}; // total nu's py
  size_t indx{0};  // quark counter 

  LV lv_q1    = ps.lv(PSPart::q1);
  LV lv_qbar1 = ps.lv(PSPart::qbar1);
  LV lv_b1    = ps.lv(PSPart::b1);
  LV lv_q2    = ps.lv(PSPart::q2);
  LV lv_qbar2 = ps.lv(PSPart::qbar2);
  LV lv_b2    = ps.lv(PSPart::b2);
  LV lv_b     = ps.lv(PSPart::b);
  LV lv_bbar  = ps.lv(PSPart::bbar);

  if( debug_code&DebugVerbosity::integration ){
    cout << "\t\tFilling m..." << endl;
    cout << "\t\tCheck masses: m(W1)=" << (lv_q1+lv_qbar1).M() << ", m(t1)=" << (lv_q1+lv_qbar1+lv_b1).M()
	 << ", m(W2)=" << (lv_q2+lv_qbar2).M() << ", m(t2)=" << (lv_q2+lv_qbar2+lv_b2).M() 
	 << ", m(H)=" << (lv_b+lv_bbar).M() << endl;
  }

  m *= t_decay_amplitude(lv_q1, lv_qbar1, lv_b1);
  m *= t_decay_amplitude(lv_q2, lv_qbar2, lv_b2);
  m *= (hypo==Hypothesis::TTH ? H_decay_amplitude(lv_b, lv_bbar) : 1.0 );
  m *= scattering( lv_q1+lv_qbar1+lv_b1, lv_q2+lv_qbar2+lv_b2, lv_b, lv_bbar);
  m *= pdf( lv_q1+lv_qbar1+lv_b1+lv_q2+lv_qbar2+lv_b2+lv_b+lv_bbar );

  if( debug_code&DebugVerbosity::integration ){
    cout << "\t\tFilling p..." << endl;
  }

  map<MEM::PSPart, MEM::GenPart>::const_iterator p;
  for( p = ps.begin() ; p != ps.end() ; ++p ){
    if( isLepton  ( p->second.type ) ) continue;
    if( isNeutrino( p->second.type ) ){
      nu_x += p->second.lv.Px();
      nu_y += p->second.lv.Py();
      continue;
    }      
    // observables and generated-level quantities
    // if the parton is matched, test value of jet energy
    double e_gen  {p->second.lv.E()}; 
    double eta_gen{p->second.lv.Eta()};     
    double e_rec  = perm[indx]>=0 ? obs_jets[perm[indx]]->p4().E() : 0.;

    // build x,y vectors 
    double y[1] = { e_rec };
    double x[2] = { e_gen, eta_gen };
    w *= transfer_function( y, x, p->second.type ); 

    // increment quark counter
    ++indx;
  }

  if( fs!=FinalState::HH ){
    double y[2] = { obs_mets[0]->p4().Px(), obs_mets[0]->p4().Py() };
    double x[2] = { nu_x, nu_y };
    w *= transfer_function( y, x, TFType::MET );
  }

  if( debug_code&DebugVerbosity::integration ){
    cout << "\tIntegrand::probability(): END" << endl;
  }

  return m*w;
}


double MEM::Integrand::scattering(const TLorentzVector&, const TLorentzVector&, const TLorentzVector&, const TLorentzVector&) const{
  double p{1.};
  return p;
}

double MEM::Integrand::pdf(const TLorentzVector& lv) const{
  double p{1.};
  return p;
}

double MEM::Integrand::t_decay_amplitude(const TLorentzVector&, const TLorentzVector&, const TLorentzVector&) const{
  double p{1.};
  return p;
}

double MEM::Integrand::H_decay_amplitude(const TLorentzVector&, const TLorentzVector&) const{
  double p{1.};
  return p;
}

double MEM::Integrand::solve(const LV& p4_w, const double& DM2, const double& M, const TVector3& e_b, const double& target ) const {

  double a     = DM2/p4_w.E();
  double b     = TMath::Cos(p4_w.Angle(e_b));
  if( M<1e-03 ){
    if( debug_code&DebugVerbosity::integration ){
      cout << "\t\tUse masless formula: " << a << "/(1-" << b << ")=" << a/(1-b) << endl;  
    }
    return  (b<1. ? a/(1-b) : numeric_limits<double>::max()); 
  }
  
  // use adimensional 'a', account for velocity<1
  a /= M;
  b *= p4_w.Beta();
  double a2    = a*a;
  double b2    = b*b;

  // this is needed to test the solutions
  double discr = a2 + b2 - a2*b2 - 1; 

  // make sure there is >0 solutions
  if( (a2 + b2 - 1)<0. ){
    if( debug_code&DebugVerbosity::integration ){
      cout << "\t\t(a2 + b2 - 1)<0. return max()" << endl;
    }
    return numeric_limits<double>::max();
  }

  // the roots
  double g_p = (a + TMath::Abs(b)*sqrt(a2 + b2 - 1))/(1-b2);
  double g_m = (a - TMath::Abs(b)*sqrt(a2 + b2 - 1))/(1-b2);

  // make sure this is >1 ( g_m < g_p )
  if( g_p < 1.0 ){
    if( debug_code&DebugVerbosity::integration ){
      cout << "\t\tg_p=" << g_p << ": return max()" << endl;
    }
    return numeric_limits<double>::max();
  }

  // remove unphysical root
  if( g_m < 1.0) g_m = g_p;

  // test for the roots
  switch( b>0 ){
  case true :
    if( discr<0 ){
      if( debug_code&DebugVerbosity::integration ){
	cout << "\t\tb>0 AND discr<0: return root closest to target" << endl;
      }
      return ( TMath::Abs(target-g_p)<TMath::Abs(target-g_m) ? g_p*M : g_m*M );
    }
    if( debug_code&DebugVerbosity::integration ){
      cout << "\t\tb>0 AND discr>0: return g_p*M" << endl;
    }
    return g_p*M;
    break;
  case false:
    if( discr>0 ){
      if( debug_code&DebugVerbosity::integration ){
	cout << "\t\tb<0 AND discr>0: return g_m*M" << endl;
      }
      return g_m*M;
    }
    break;
  }

  if( debug_code&DebugVerbosity::integration ){
    cout << "\tIntegrand::solve(): START" << endl;
  }

  return numeric_limits<double>::max();
}
