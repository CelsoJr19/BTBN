@echo off
title Trade Bot Binance

echo Carregando configuracoes de API...
CALL config.bat

echo.
echo Iniciando o bot...
echo.

botzin.exe

echo.
echo O bot foi finalizado. Pressione qualquer tecla para fechar esta janela.

pause > nul
