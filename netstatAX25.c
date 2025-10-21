/*
 * netstatAX25
 *
 * Copyright (C) 2025 Bernard Pidoux, f6bvp / ai7bg
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
/*******************************************************************************
 * netstatAX25 - AX.25 Kernel Socket Status Utility
 *
 * Copyright (C) 2025 Bernard Pidoux, F6BVP / AI7BG
 *
 * This utility displays active AX.25 sockets, providing extensive compatibility
 * across various Linux kernel versions by handling different /proc/net/ax25
 * file formats and indexing schemes.
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#define PROC_FILE "/proc/net/ax25"
#define LINE_MAX_LEN 2048
#define MAX_FIELDS 32

// Global flag to track if the file has a header (New Format) or not (Old Format).
int has_header = 0; 

// AX.25 Compatible State Definitions
const char *ax25_states_compat[] = {
    "LISTENING",   
    "SABM SENT",   
    "DISC SENT", // Unused, but kept for array consistency
    "ESTABLISHED", 
    "RECOVERY",    
    "UNKNOWN_STATE"
};

/**
 * @brief Converts the numeric AX.25 state index to a compatible string.
 */
const char *get_ax25_state(int state) {

//    printf(" State %d ",state);

    if (state == 0) return ax25_states_compat[0]; // LISTENING
    if (state == 3) return ax25_states_compat[3]; // ESTABLISHED

    // Map kernel states (1-7) to the 5 compatible strings
    switch(state) {
        case 1:
        case 2:
            return ax25_states_compat[1]; // SABM SENT (Connect/Setup states)
        case 4:
        case 5:
        case 6:
        case 7:
            return ax25_states_compat[4]; // RECOVERY (Release/Disconnected states)
        default:
            return ax25_states_compat[5]; // UNKNOWN_STATE
    }
}

/**
 * @brief Parses and prints a data line from /proc/net/ax25.
 */
void parse_netstat_format(char *line) {
    char *fields[MAX_FIELDS];
    char *token;
    char *saveptr;
    int field_count = 0;
    
    // --- SÉLECTION DYNAMIQUE DES INDICES ET DU SEUIL ---
    int send_q_idx;
    int recv_q_idx;
    int min_fields; // Le nombre minimum de champs requis

if (has_header) {
    // --- NOUVEAU FORMAT (21 champs au total) ---
    send_q_idx = 18; 
    recv_q_idx = 19; 
    min_fields = 21; // Seulement 21 champs sont requis pour accéder à l'index 20
} else {
    // --- ANCIEN FORMAT (Votre noyau, 24 champs au total) ---
    send_q_idx = 21; 
    recv_q_idx = 22; 
    min_fields = 24; 
}

//    printf("%s", line);
 
    char *line_copy = strdup(line);
    if (!line_copy) { perror("strdup"); return; }
    
    // Split the line into fields
    token = strtok_r(line_copy, " \t", &saveptr);
    while (token != NULL && field_count < MAX_FIELDS) {
        fields[field_count] = token;
        field_count++;
        token = strtok_r(NULL, " \t", &saveptr);
    }
  
    // --- 2. VÉRIFICATION DYNAMIQUE (Utilisation du seuil) ---
    if (field_count < min_fields) { 
        free(line_copy);
        return;
    }

    // --- Retrieve main fields ---
    char *dest_call_raw = fields[3]; 
    char *source_call = fields[2];
    char *interface = fields[1];
    
    // --- DIGIPEATER (Digis) LOGIC ---
    char digis_buffer[25]; 
    const char *digi1_str = NULL;
    const char *digi2_str = NULL;
    
    if (!has_header) {
        // --- OLD FORMAT (Comma-separated digis) ---
        char *comma_sep = strchr(dest_call_raw, ',');
        
        if (comma_sep != NULL) {
            // Comma found: Parse Digis from fields[3].
            *comma_sep = '\0'; 
            char *digi_chain = comma_sep + 1;
            char *second_comma = strchr(digi_chain, ',');
            
            if (second_comma != NULL) {
                *second_comma = '\0'; 
                digi2_str = second_comma + 1;
            }
            digi1_str = digi_chain;
        }
    } else {
        // --- NEW FORMAT (Explicit fields [4] and [5] for Digis) ---
        digi1_str = (field_count > 4 && strcmp(fields[4], "*") != 0) ? fields[4] : NULL;
        digi2_str = (field_count > 5 && strcmp(fields[5], "*") != 0) ? fields[5] : NULL;
    }

    // --- FORMAT OUTPUT STRING ---
    if (digi1_str) {
        if (digi2_str) {
            snprintf(digis_buffer, sizeof(digis_buffer), "%-10s%-10s", digi1_str, digi2_str);
        } else {
            snprintf(digis_buffer, sizeof(digis_buffer), "%-10s    -     ", digi1_str);
        }
    } else {
        snprintf(digis_buffer, sizeof(digis_buffer), "*"); 
    }
    
    char *digis = digis_buffer; 
    int state_num, vs, vr;
    // --- STATE, VS/VR LOGIC (Indices) ---
    if (!has_header) {
    // OLD FORMAT STATE: Index 4
        state_num = atoi(fields[4]);
    // VS/VR: Indices 6 (Vs) and 7 (Vr)
        vs = atoi(fields[6]);
        vr = atoi(fields[7]);
    }
    else {
    // NEW FORMAT STATE: Index 6
        state_num = atoi(fields[6]);
    // VS/VR: Indices 7 (Vs) and 8 (Vr)
        vs = atoi(fields[7]);
        vr = atoi(fields[8]);
    }    

    const char *state_str = get_ax25_state(state_num);
    char vr_vs[8];
    snprintf(vr_vs, sizeof(vr_vs), "%03d/%03d", vs, vr);

    // Send-Q and Recv-Q (Lecture avec les indices dynamiques)
    char *send_q = fields[send_q_idx]; 
    char *recv_q = fields[recv_q_idx]; 
    
    // Structured Netstat Output (8 columns)
    printf("%-12s %-12s %-7s %-12s %-21s %-7s %7s %7s\n",
           dest_call_raw, 
           source_call,
           interface,
           state_str,
           digis,           
           vr_vs,
           send_q,          
           recv_q           
    );
    
    free(line_copy);
}

/**
 * @brief Main function for reading, checking, and displaying AX.25 data.
 */
int main() {
    FILE *file;
    char line[LINE_MAX_LEN];
    int line_count = 0;

    // --- 1. File Opening and Error Check ---
    file = fopen(PROC_FILE, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Cannot open file %s.\n", PROC_FILE);
        
        switch (errno) {
            case ENOENT:
                fprintf(stderr, "Diagnostic: File not found. This usually means the AX.25 kernel module is NOT loaded.\n");
                fprintf(stderr, "Action: Try loading the module (e.g., 'sudo modprobe ax25') and ensure AX.25 is configured.\n");
                break;
            case EACCES:
                fprintf(stderr, "Diagnostic: Permission denied. The user running this program cannot read the file.\n");
                fprintf(stderr, "Action: Try running the program with root privileges (e.g., 'sudo ./netstat_procutils') or check file permissions.\n");
                break;
            default:
                fprintf(stderr, "System Error: %s\n", strerror(errno));
                break;
        }
        return EXIT_FAILURE;
    }

    // --- 2. Check for header and count lines for activity ---
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        fprintf(stdout, "Warning: File %s is empty. Is AX25 active ?\n", PROC_FILE);
        return EXIT_SUCCESS;
    }
    
    char *p = line;
    while(isspace(*p)) p++; 
    
    if (!isdigit(*p)) { 
        has_header = 1; 
        line_count = 1;
    } else {
        has_header = 0; 
        line_count = 1;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        line_count++;
    }
    
    fclose(file); 

    // --- 3. AX.25 Activity Check (Warning Logic) ---
    if (line_count <= (has_header ? 1 : 0)) {
        fprintf(stdout, "Warning: No active AX.25 connections currently.\n");
        return EXIT_SUCCESS;
    }

    // --- 4. Second pass: Read and display data ---
    file = fopen(PROC_FILE, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Cannot reopen file %s: %s\n", PROC_FILE, strerror(errno));
        return EXIT_FAILURE;
    }

// Skip the header if present
    if (has_header) {
        if (fgets(line, sizeof(line), file) != NULL) {
        }
    }

    // Display formatted Netstat headers
    printf("Active AX.25 Sockets\n");
    printf("%-12s %-12s %-7s %-12s %-21s %-7s %7s %7s\n",
           "Dest", "Source", "Device", "State", "Digis", "Vs/Vr", "Send-Q", "Recv-Q");
    
    // Loop through remaining data lines
    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;

        parse_netstat_format(line);
    }
    
    fclose(file);
    return EXIT_SUCCESS;
}
