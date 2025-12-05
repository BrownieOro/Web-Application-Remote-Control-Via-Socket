@echo off
cd /d "%~dp0"
cd Client
npm install ws
start Web.html
node node.js
pause