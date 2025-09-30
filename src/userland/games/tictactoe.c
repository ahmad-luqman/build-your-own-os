/*
 * Tic-Tac-Toe Game - Interactive game for MiniOS
 * Demonstrates interactive user programs and game logic
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// User I/O functions
int user_printf(const char *format, ...);
int user_getchar(void);
int user_puts(const char *str);

// Game board
char board[3][3];
char current_player = 'X';

// Initialize game board
void init_board(void) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            board[i][j] = ' ';
        }
    }
}

// Display the game board
void display_board(void) {
    user_puts("  Current Board:");
    user_puts("     0   1   2");
    user_puts("   +---+---+---+");
    
    for (int i = 0; i < 3; i++) {
        user_printf(" %d | %c | %c | %c |\n", i, board[i][0], board[i][1], board[i][2]);
        user_puts("   +---+---+---+");
    }
    user_puts("");
}

// Check if position is valid and empty
int is_valid_move(int row, int col) {
    return (row >= 0 && row < 3 && col >= 0 && col < 3 && board[row][col] == ' ');
}

// Make a move
void make_move(int row, int col) {
    board[row][col] = current_player;
}

// Check for winner
char check_winner(void) {
    // Check rows
    for (int i = 0; i < 3; i++) {
        if (board[i][0] != ' ' && board[i][0] == board[i][1] && board[i][1] == board[i][2]) {
            return board[i][0];
        }
    }
    
    // Check columns
    for (int j = 0; j < 3; j++) {
        if (board[0][j] != ' ' && board[0][j] == board[1][j] && board[1][j] == board[2][j]) {
            return board[0][j];
        }
    }
    
    // Check diagonals
    if (board[0][0] != ' ' && board[0][0] == board[1][1] && board[1][1] == board[2][2]) {
        return board[0][0];
    }
    
    if (board[0][2] != ' ' && board[0][2] == board[1][1] && board[1][1] == board[2][0]) {
        return board[0][2];
    }
    
    return ' ';  // No winner
}

// Check if board is full
int is_board_full(void) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i][j] == ' ') {
                return 0;
            }
        }
    }
    return 1;
}

// Switch player
void switch_player(void) {
    current_player = (current_player == 'X') ? 'O' : 'X';
}

// Get player input
void get_move(int *row, int *col) {
    char input[10];
    int i = 0;
    int ch;
    
    while (1) {
        user_printf("Player %c, enter your move (row col): ", current_player);
        
        // Read input
        i = 0;
        while (i < 9 && (ch = user_getchar()) != '\n' && ch != EOF) {
            input[i++] = ch;
        }
        input[i] = '\0';
        
        // Parse input (simple parser for "row col" format)
        if (i >= 3 && input[1] == ' ') {
            *row = input[0] - '0';
            *col = input[2] - '0';
            
            if (is_valid_move(*row, *col)) {
                return;
            }
        }
        
        user_puts("Invalid move! Enter row and column (0-2) separated by space.");
    }
}

int main(int argc, char *argv[]) {
    user_puts("MiniOS Tic-Tac-Toe Game");
    user_puts("=======================");
    user_puts("Enter moves as 'row col' (0-2 for both)");
    user_puts("Player X goes first!");
    user_puts("");
    
    init_board();
    
    while (1) {
        display_board();
        
        int row, col;
        get_move(&row, &col);
        make_move(row, col);
        
        char winner = check_winner();
        if (winner != ' ') {
            display_board();
            user_printf("Player %c wins! Congratulations!\n", winner);
            break;
        }
        
        if (is_board_full()) {
            display_board();
            user_puts("It's a tie! Good game!");
            break;
        }
        
        switch_player();
    }
    
    user_puts("\nThanks for playing Tic-Tac-Toe on MiniOS!");
    return 0;
}

// Stub implementations for user I/O
int user_printf(const char *format, ...) {
    return 0;
}

int user_getchar(void) {
    return 0;
}

int user_puts(const char *str) {
    return 0;
}