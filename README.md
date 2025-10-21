# netstatAX25
AX.25 Kernel Socket Status Utility
## Overview

`netstatAX25` is a specialized utility designed to display active AX.25 sockets by parsing the `/proc/net/ax25` file. It is specifically engineered for **robustness and backward compatibility** across a wide range of Linux kernels, including older systems where the standard file format and data indexing may be inconsistent.

---

## Key Features and Compatibility

This utility goes beyond simple file parsing to ensure reliable output on diverse systems:

### 1. Dual Format Support

* **Problem:** The format of `/proc/net/ax25` has changed over time (e.g., the inclusion or removal of a text header).
* **Solution:** `netstatAX25` handles **both headered and headerless formats**, ensuring data extraction is correct whether the data starts immediately or after a header line.

### 2. Compatible AX.25 States

* **Problem:** The kernel reports AX.25 states as numerical codes (e.g., 1, 4).
* **Solution:** These numerical states are mapped to **legacy, user-friendly strings** (e.g., `'SABM SENT'`, `'ESTABLISHED'`, `'RECOVERY'`) for immediate user familiarity.

### 3. Dynamic Field Indexing

* **Problem:** In some older kernels, the position (index) of key data fields shifts unexpectedly.
* **Solution:** The utility uses logic to correct for these shifts, ensuring critical data like **State, Vs, Vr, Send-Q, and Recv-Q** are read from the statistically correct positions, regardless of minor kernel variations.

### 4. Robust Digipeater Parsing

* **Feature:** Correctly extracts comma-separated digipeaters from the destination field.
* **Clarity:** Displays `*` reliably when no digipeaters are present, maintaining clean output.

### 5. Improved Error Handling

* **Feature:** Provides **specific, diagnostic messages** for common issues encountered during file access or parsing, aiding in quicker troubleshooting.

---

## Installation

Please see the **[INSTALLING](INSTALLING)** file for detailed steps on compilation and installation.

1.  **Compile:** Run `make`
2.  **Install:** Run `sudo make install`
