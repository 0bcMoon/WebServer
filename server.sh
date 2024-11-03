#!/bin/bash
./webserv & 
sleep 2
curl -i http://localhost:8080/main.php
fg 
