#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#define PROC_AX25_FILE "/proc/net/ax25"
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
    
    // --- DYNAMIC SELECTION OF INDICES AND THRESHOLD ---
    int send_q_idx;
    int recv_q_idx;
    int min_fields; // The minimum number of required fields

    if (has_header) {
        // --- NEW FORMAT (Total 21 fields) ---
        send_q_idx = 18;  
        recv_q_idx = 19;  
        min_fields = 21; 
    } else {
        // --- OLD FORMAT (Total 24 fields in kernel documentation) ---
        // Note: Indices for Vs, Vr, Va were adjusted based on your input.
        send_q_idx = 21;  
        recv_q_idx = 22;  
        min_fields = 24;  
    }

    char *line_copy = strdup(line);
    if (!line_copy) { perror("strdup"); return; }
    
    // Split the line into fields
    token = strtok_r(line_copy, " \t", &saveptr);
    while (token != NULL && field_count < MAX_FIELDS) {
        fields[field_count] = token;
        field_count++;
        token = strtok_r(NULL, " \t", &saveptr);
    }
    
    // --- 2. DYNAMIC VALIDATION (Using the threshold) ---
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
            snprintf(digis_buffer, sizeof(digis_buffer), "%-10s    -      ", digi1_str);
        }
    } else {
        snprintf(digis_buffer, sizeof(digis_buffer), "*");  
    }
    
    char *digis = digis_buffer;  
    int state_num, vs, vr, va;
    
    // --- STATE, VS/VR/VA LOGIC (Indices) ---
    if (!has_header) {
        // OLD FORMAT - Indices corrected based on user input
        // STATE: Index 4
        state_num = atoi(fields[4]);
        
        // VS/VR/VA: Indices 5, 6, 7
        vs = atoi(fields[5]); // Corrected from fields[6]
        vr = atoi(fields[6]); // Corrected from fields[7]
        va = atoi(fields[7]); // Corrected from fields[8]
    }
    else {
        // NEW FORMAT
        // STATE: Index 6
        state_num = atoi(fields[6]);
        
        // VS/VR/VA: Indices 7, 8, 9
        vs = atoi(fields[7]);
        vr = atoi(fields[8]);
        va = atoi(fields[9]);
    }    

    const char *state_str = get_ax25_state(state_num);
    
    char vs_vr_va[12];
    
    // Vs/Vr/Va formatting
    snprintf(vs_vr_va, sizeof(vs_vr_va), "%03d/%03d/%03d", vs, vr, va);
    
    // Send-Q and Recv-Q (Read with dynamic indices)
    char *send_q = fields[send_q_idx];  
    char *recv_q = fields[recv_q_idx];  
    
    // Structured Netstat Output (8 columns)
    printf("%-12s %-12s %-7s %-12s %-21s %-12s %7s %7s\n",
        dest_call_raw,  
        source_call,
        interface,
        state_str,
        digis,         
        vs_vr_va,
        send_q,          
        recv_q             
    );
    
    free(line_copy);
}

/**
 * @brief Main function for reading, checking, and displaying AX.25 data.
 */
int main() {
    FILE *pipe_fp = NULL;
    char line[LINE_MAX_LEN];
    int line_count = 0;
    
    // The command to read the file
    char command[sizeof("/bin/cat ") + sizeof(PROC_AX25_FILE)];
    sprintf(command, "/bin/cat %s", PROC_AX25_FILE);

    // --- 1. First Pass: Check for header and count lines ---
    
    // Use popen to read the file via the 'cat' command
    if ((pipe_fp = popen(command, "r")) == NULL) {
        fprintf(stderr, "Error: Cannot open pipe for command '%s'.\n", command);
        // popen errors often indicate inability to execute the shell or cat.
        fprintf(stderr, "Diagnostic: Ensure '/bin/cat' is executable and your system environment is correct.\n");
        return EXIT_FAILURE;
    }

    // Check for header and count lines for activity
    if (fgets(line, sizeof(line), pipe_fp) == NULL) {
        pclose(pipe_fp); // Close the pipe
        fprintf(stdout, "Warning: File %s is empty or unreadable. Is AX25 active ?\n", PROC_AX25_FILE);
        return EXIT_SUCCESS;
    }
    
    char *p = line;
    while(isspace(*p)) p++; // Skip leading whitespace
    
    // Determine the presence of the header
    if (!isdigit(*p)) { 
        has_header = 1; 
        line_count = 1;
    } else {
        has_header = 0; 
        line_count = 1;
    }

    // Count remaining lines
    while (fgets(line, sizeof(line), pipe_fp) != NULL) {
        line_count++;
    }
    
    // IMPORTANT: Close the pipe after the first read
    pclose(pipe_fp); 

    // --- 2. AX.25 Activity Check (Warning Logic) ---
    if (line_count <= (has_header ? 1 : 0)) {
        fprintf(stdout, "Warning: No active AX.25 connections currently.\n");
        return EXIT_SUCCESS;
    }

    // --- 3. Second Pass: Reopen the pipe to read and display data ---
    // We MUST re-execute popen() because a pipe is not rewindable.
    if ((pipe_fp = popen(command, "r")) == NULL) {
        fprintf(stderr, "Error: Cannot reopen pipe for command '%s': %s\n", command, strerror(errno));
        return EXIT_FAILURE;
    }

    // Skip the header if present
    if (has_header) {
        if (fgets(line, sizeof(line), pipe_fp) == NULL) {
             pclose(pipe_fp);
             fprintf(stderr, "Error: Pipe closed unexpectedly after header check.\n");
             return EXIT_FAILURE;
        }
    }

    // Display formatted Netstat headers
    printf("Active AX.25 Sockets\n");
    printf("%-12s %-12s %-7s %-12s %-21s %-12s %7s %7s\n",
        "Destination", "Source", "Device", "State", "Digipeaters", "Vs/Vr/Va", "Send-Q", "Recv-Q");    
        
    // Loop through remaining data lines
    while (fgets(line, sizeof(line), pipe_fp) != NULL) {
        line[strcspn(line, "\n")] = 0; // Removes the newline character
        if (strlen(line) == 0) continue;

        parse_netstat_format(line);
    }
    
    // IMPORTANT: Close the pipe after the second read
    pclose(pipe_fp);
    return EXIT_SUCCESS;
}
