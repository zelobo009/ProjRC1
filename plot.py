import matplotlib.pyplot as plt

# ---------- Parameters ----------
C = [4800, 9600, 19200, 115200]        # Channel capacities (bps)
Tprop = [0, 1014e-6, 100000e-6, 999999e-6]  # Propagation delays (s)
BER = [0, 1e-6, 1e-5, 1e-4]            # Bit error rates
L = [500, 1000, 1500, 1800]            # Frame lengths (bytes)

# ---------- Fixed parameters ----------
R = 9600  # bit rate in bps

# ---------- Efficiency calculation ----------
def calc_efficiency(Tprop_list, L_list, R):
    """Compute S = 1 / (1 + 2a) where a = Tprop / Tf, Tf = L*8/R."""
    S = []
    for Tprop, L_bytes in zip(Tprop_list, L_list):
        Tf = (L_bytes * 8) / R
        a = Tprop / Tf
        S.append(1 / (1 + 2*a))
    return S

def calc_efficiency_with_BER(Tprop, L, R, BER):
    """Compute S considering BER."""
    S = []
    for ber in BER:
        Pf = 1 - (1 - ber)**(L * 8)  # Frame error probability
        Tf = (L * 8) / R
        a = Tprop / Tf
        S_eff = (1 - Pf) / (1 + 2*a)
        S.append(S_eff)
    return S

def calc_efficiency_vs_C(Tprop, L_bytes, C_list):
    S = []
    for C in C_list:
        Tf = (L_bytes * 8) / C
        a = Tprop / Tf
        S.append(1 / (1 + 2*a))
    return S



# ---------- Compute efficiencies ----------

# S vs C: assume fixed Tprop=0.01s, L=1000 bytes
S_C = calc_efficiency_vs_C(0.01, 1000, C)


# S vs Tprop: use fixed frame length L=1000 bytes
S_Tprop = calc_efficiency(Tprop, [1000]*len(Tprop), R)

# S vs BER: assume fixed Tprop=0.01s, L=1000 bytes
S_BER = calc_efficiency_with_BER(0.01, 1000, R, BER)

# S vs L: assume fixed Tprop=0.01s
S_L = calc_efficiency([0.01]*len(L), L, R)

# ---------- Plot ----------
fig, axes = plt.subplots(2, 2, figsize=(12, 8))

# S vs C
axes[0,0].plot(C, S_C, 'o-', color='tab:blue')
axes[0,0].set_xlabel('Channel capacity C (bps)')
axes[0,0].set_ylabel('Efficiency S')
axes[0,0].set_title('S vs Channel Capacity')

# S vs Tprop
axes[0,1].plot([t*1e6 for t in Tprop], S_Tprop, 'o-', color='tab:green')
axes[0,1].set_xlabel('Propagation delay Tprop (µs)')
axes[0,1].set_ylabel('Efficiency S')
axes[0,1].set_title('S vs Propagation Delay')

# S vs BER
axes[1,0].semilogx(BER, S_BER, 'o-', color='tab:red')
axes[1,0].set_xlabel('Bit Error Rate (BER)')
axes[1,0].set_ylabel('Efficiency S')
axes[1,0].set_title('S vs BER')

# S vs L
axes[1,1].plot(L, S_L, 'o-', color='tab:orange')
axes[1,1].set_xlabel('Frame length L (bytes)')
axes[1,1].set_ylabel('Efficiency S')
axes[1,1].set_title('S vs Frame Length')
axes[1,1].set_ylim(0.8, 1) 

plt.tight_layout()
plt.show()
