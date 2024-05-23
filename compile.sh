#!/bin/bash

echo "Compilando programa"
g++ zombot.cpp -o ZomBot -I/usr/local/include -L/usr/local/lib -lTgBot -lboost_system -lssl -lcrypto -lpthread -lcurl -lrconpp -lconfig++ -ldpp