# tcp-congestion-control-comparison-ns3
Comparative study of TCP NewReno and TCP Vegas congestion control algorithms using NS-3.43

# Comparative Analysis of TCP Congestion Control Methods

### 🧠 Overview  
This project presents a comparative study of **TCP NewReno** (loss-based) and **TCP Vegas** (delay-based) congestion control algorithms using **Network Simulator 3 (NS-3.43)**.  
The simulations evaluate each variant’s performance under varying traffic loads, queue management schemes (DropTail vs RED), and in the presence of competing UDP traffic.

---

### 🎯 Objectives
- Analyze TCP behavior in mixed TCP–UDP network conditions.  
- Implement and simulate **TCP NewReno** and **TCP Vegas** in NS-3.43.  
- Compare performance metrics including **throughput**, **delay**, and **packet loss**.  
- Evaluate the impact of **DropTail** and **RED** queue disciplines.  

---

### ⚙️ Tools & Technologies
- **NS-3.43** – Network simulation environment  
- **C++** – Implementation language for NS-3 scripts  
- **Ubuntu 22.04 LTS** – Execution environment  
- **FlowMonitor Module** – Data collection  
- **Gnuplot / Matplotlib** – Result visualization  

---

### 🧩 Methodology
Two scenarios were simulated:

**Scenario 1 – TCP–UDP Interaction (5 Nodes)**  
- One TCP flow (Vegas/NewReno) and one UDP CBR flow  
- Link bandwidth: 10 Mbps, delay: 10 ms  
- Queue type: DropTail  
- CBR rate varied from 1 Mbps to 10 Mbps  

**Scenario 2 – Multiple TCP + UDP Flows (9 Nodes)**  
- Two TCP flows (Vegas/NewReno and NewReno) and one UDP CBR flow  
- Queue type: DropTail and RED  
- Simulation time: 100 seconds  

---

### 📊 Performance Metrics
- **Throughput (Mbps)** = Total bits received / Simulation time  
- **Average Delay (ms)** = Σ(receive_time - send_time) / total_packets  
- **Packet Loss Rate (%)** = ((Sent - Received) / Sent) × 100  

---

### 📈 Results Summary
- **TCP Vegas** achieved **lower delay and loss**, adapting early to congestion.  
- **TCP NewReno** offered **higher throughput**, but at the cost of increased delay and packet drops.  
- **RED queue discipline** outperformed DropTail by maintaining smoother delay and preventing sudden losses.  

---

### 🧾 Conclusion
The study demonstrates that **TCP Vegas** is suitable for delay-sensitive applications, while **TCP NewReno** performs better in throughput-demanding scenarios. Queue discipline significantly influences overall network performance.

---


### 📚 References
1. Ahmad Abadleh et al., *Comparative Analysis of TCP Congestion Control Methods*, IEEE ICICS 2022.  
2. Michele Polese et al., *A Survey on Recent Advances in Transport Layer Protocols*, IEEE Communications Surveys & Tutorials, 2019.  
3. NS-3.43 Documentation – [https://www.nsnam.org/documentation](https://www.nsnam.org/documentation)

---


