/*
 * Hasan Buğra Uslu G211210009  2C
 * Tarık Kalyoncu  G211210030 2C
 * 
 * main.c
 * Linux Uygulaması Ana Kod Dosyası
 */

#include "shell.h"
#include "stdio.h"

int main() {
    // Kullanıcıya hoş geldiniz mesajı yazdır
    printf("Linux Kabuk'a hoş geldiniz! Çıkmak için 'quit' yazın.\n");
    
    // Kabuk programını çalıştır
    run_shell();
    
    // Program başarılı şekilde sonlanır
    return 0;
}



