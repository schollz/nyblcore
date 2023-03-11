breakbeat:
	rm -rf output*wav
	sox bpm150_2.wav -r 6440 -c 1 -b 8  0.wav trim 0 1
	ls -l 0.wav
	sox 0.wav output.wav trim 0 0.2 : newfile : restart
	go run make-breakbeat-table.go
	
breakbeat2:
	sox bpm180.wav -r 6440 -c 1 -b 8  0.wav trim 0 1
	ls -l 0.wav
	sox 0.wav output.wav trim 0 0.1667 : newfile : restart
	rm -f output007.wav
	go run make-breakbeat-table.go
	
breakbeat3:
	rm -rf output*wav
	sox bpm150_3.wav -r 4830 -c 1 -b 8 0.wav norm lowpass 2415 trim 0 1.2 dither
	ls -l 0.wav
	sox 0.wav output.wav trim 0 0.2 : newfile : restart
	go run make-breakbeat-table.go

breakbeat4:
	rm -rf output*wav
	sox bpm150_4.wav -r 4830 -c 1 -b 8 0.wav norm lowpass 2415 trim 0 1.0 dither
	ls -l 0.wav
	sox 0.wav output.wav trim 0 0.2 : newfile : restart
	go run make-breakbeat-table.go


breakbeat5:
	rm -rf output*wav
	sox bpm150_5.wav -r 4830 -c 1 -b 8 0.wav norm lowpass 2415 trim 0 1.2 dither
	ls -l 0.wav
	sox 0.wav output.wav trim 0 0.2 : newfile : restart
	go run make-breakbeat-table.go

breakbeat6:
	rm -rf output*wav
	sox bpm200.wav -r 2415 -c 1 -b 8 0.wav norm lowpass 2415 trim 0 2.4 dither
	ls -l 0.wav
	sox 0.wav output.wav trim 0 0.6 : newfile : restart
	go run make-breakbeat-table.go
