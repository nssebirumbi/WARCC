avrdude -p m256rfr2 -c stk500v2 -P "COM53" -b 38400 -e  -U flash:w:gen3sink-15s.hex