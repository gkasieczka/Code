import ROOT
ROOT.gSystem.Load("libFWCoreFWLite")
ROOT.gROOT.ProcessLine('AutoLibraryLoader::enable();')
ROOT.gSystem.Load("libFWCoreFWLite")
ROOT.gSystem.Load("libCintex")
ROOT.gROOT.ProcessLine('ROOT::Cintex::Cintex::Enable();')
ROOT.gSystem.Load("libTTHMEIntegratorStandalone")
from ROOT import MEM
from ROOT import TLorentzVector
import math

cfg = MEM.MEMConfig()
cfg.transfer_function_method = MEM.TFMethod.External
mem = MEM.Integrand(MEM.output, cfg)
print mem

def add_obj(mem, typ, **kwargs):

    if kwargs.has_key("p4s"):
        pt, eta, phi, mass = kwargs.pop("p4c")
        v = TLorentzVector()
        v.SetPtEtaPhiM(pt, eta, phi, mass);
    elif kwargs.has_key("p4c"):
        v = TLorentzVector(*kwargs.pop("p4c"))
    obsdict = kwargs.pop("obsdict", {})

    o = MEM.Object(v, typ)

    t1 = kwargs.get("tf", None)

    if t1 != None:
        o.addTransferFunction(MEM.TFType.qReco, t1)
        o.addTransferFunction(MEM.TFType.bReco, t1)

        o.addTransferFunction(MEM.TFType.qLost, t1)
        o.addTransferFunction(MEM.TFType.bLost, t1)

    for k, v in obsdict.items():
        o.addObs(k, v)
    mem.push_back_object(o)

# add_obj(mem,
#     MEM.ObjectType.Jet, p4c=(50, 0, 10, math.sqrt(50*50+10*10)),
#     obsdict={MEM.Observable.BTAG: 0.0}
# )

t1 = ROOT.TF1("fb","[0]*exp(-0.5*((x-[1])/[2])**2)",0,500)
t1.SetParameter(0, 1.0 / 100/math.sqrt(2*3.1415)) #normalization
t1.SetParameter(1, 100) #mean
t1.SetParameter(2, math.sqrt(2)*100) #unc


add_obj(mem,
    MEM.ObjectType.Jet, p4c=(0, 50, 20, math.sqrt(50*50+20*20)),
    obsdict={MEM.Observable.BTAG: 1.0}, tf=t1
)
# add_obj(mem,
#     MEM.ObjectType.Jet, p4c=(30, 30, 40, math.sqrt(30*30+30*30+40*40)),
#     obsdict={MEM.Observable.BTAG: 1.0}
# )
add_obj(mem,
    MEM.ObjectType.Jet, p4c=(70, 20, 10, math.sqrt(70*70+20*20+10*10)),
    obsdict={MEM.Observable.BTAG: 1.0}
)
add_obj(mem,
    MEM.ObjectType.Jet, p4c=(20, 50, 10, math.sqrt(20*20+50*50+10*10)),
    obsdict={MEM.Observable.BTAG: 1.0}
)
add_obj(mem,
    MEM.ObjectType.Jet, p4c=(100, 10, 20, math.sqrt(100*100 + 10*10 + 20*20)),
    obsdict={MEM.Observable.BTAG: 1.0}
)
add_obj(mem,
    MEM.ObjectType.Lepton, p4c=(70, 10, 20, math.sqrt(70*70+20*20+10*10)),
    obsdict={MEM.Observable.CHARGE: 1.0}
)
add_obj(mem,
    MEM.ObjectType.Lepton, p4c=(70, -10, -20, math.sqrt(70*70+20*20+10*10)),
    obsdict={MEM.Observable.CHARGE: -1.0}
)
add_obj(mem,
    MEM.ObjectType.MET, p4c=(30, 0, 0, 30),
)
CvectorPermutations = getattr(ROOT, "std::vector<MEM::Permutations::Permutations>")

pvec = CvectorPermutations()
pvec.push_back(MEM.Permutations.BTagged)
pvec.push_back(MEM.Permutations.QUntagged)
mem.set_permutation_strategy(pvec)

mem.set_integrand(
    MEM.IntegrandType.Constant
    |MEM.IntegrandType.ScattAmpl
    |MEM.IntegrandType.DecayAmpl
    |MEM.IntegrandType.Jacobian
    |MEM.IntegrandType.PDF
    |MEM.IntegrandType.Transfer
)
mem.set_ncalls(4000);
mem.set_sqrts(13000.);


CvectorPSVar = getattr(ROOT, "std::vector<MEM::PSVar::PSVar>")
psvar_vec = CvectorPSVar()
r = mem.run(MEM.FinalState.LL, MEM.Hypothesis.TTH, psvar_vec);
print r.p
mem.next_event()
