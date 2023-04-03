package main

import (
	"fmt"
	"net/http"
	"path/filepath"

	"github.com/gin-gonic/gin"
)

func main() {
	router := gin.Default()
	// Set a lower memory limit for multipart forms (default is 32 MiB)
	router.MaxMultipartMemory = 8 << 20 // 8 MiB
	router.Static("/", "./template")
	router.POST("/", func(c *gin.Context) {
		name := c.PostForm("name")
		email := c.PostForm("email")

		// Multipart form
		form, err := c.MultipartForm()
		if err != nil {
			c.String(http.StatusBadRequest, "get form err: %s", err.Error())
			return
		}
		files := form.File["files"]


		var cmd1 []string 
		for _, file := range files {
			if strings.Contains(file,".wav") || strings.Contains(file,".mp3")  || strings.Contains(file,".flac")  || strings.Contains(file,".aif")  || strings.Contains(file,".ogg") {
				filename := filepath.Base(file.Filename)
				cmd1 = append(cmd1,filename)
				if err := c.SaveUploadedFile(file, filename); err != nil {
					c.String(http.StatusBadRequest, "upload file err: %s", err.Error())
					return
				}	
			} 
		}
		cmd1 = append(cmd1,[]string{"/tmp/0.wav"})
		cmd := exec.Command("sox", cmd1...)
		output, _ := cmd.CombinedOutput()
		fmt.Println("sox",cmd1)
		fmt.Println(string(output))
		cmd2 = []string{"/tmp/0.wav","-r","4400","-c","1","-b","8","/tmp/1.wav","norm","lowpass","2200","trim","0","1.2","dither")
		cmd = exec.Command("sox", cmd2...)
		output, _ = cmd.CombinedOutput()
		fmt.Println("sox",cmd2)
		fmt.Println(string(output))

		// TODO: if error go back to main page
		

		fmt.Println(name, email)

		bs, _ := os.ReadFile("../nyblecore/nyblcore.ino")
		
		converted := string(bs)

		data, err := convertWavToInts([]string{"/tmp/1.wav"})
		if err != nil {
			c.String(http.StatusBadRequest,"could not convert")
		}

		converted = strings.Replace(converted,"// SAMPLETABLE",data,1)

		// TODO: prepend with creation date and name of files

		c.String(http.StatusOK, converted)
	})
	router.Run(":8080")
}



func convertWavToInts(fnames []string) (ss string, err error) {
	ss = `#ifndef SAMPLE_TABLE_H_
#define SAMPLE_TABLE_H_

#define NUM_SAMPLES XXX
#define SAMPLE_SIZE UUU
#define TOTAL_SAMPLES VVV

const word retrigs[] = {
	WWW
};
const word pos[] = {
    YYY
};
const byte SAMPLE_TABLE[] PROGMEM = {
	ZZZ
};
#endif
`
	numSamples := 0
	sampleString := ""
	poses := []string{"0"}
	sliceSamples := 0
	for j, fname := range fnames {
		file, _ := os.Open(fname)
		reader := wav.NewReader(file)
		n := 0
		if j > 0 {
			sampleString += ",\n"
		}
		for {
			samples, err := reader.ReadSamples()
			if err == io.EOF {
				break
			}

			for i, sample := range samples {
				sampleString += fmt.Sprintf("%d", reader.IntValue(sample, 0))
				n++
				if i < len(samples)-1 {
					sampleString += ",\n"
				}
			}
		}
		file.Close()
		numSamples += n
		sliceSamples = n
		poses = append(poses, fmt.Sprint(numSamples))
	}
	ss = strings.Replace(ss, "XXX", fmt.Sprint(len(fnames)), 1)
	ss = strings.Replace(ss, "YYY", strings.Join(poses, ","), 1)
	ss = strings.Replace(ss, "ZZZ", sampleString, 1)
	ss = strings.Replace(ss, "UUU", fmt.Sprint(poses[1]), 1)
	ss = strings.Replace(ss, "VVV", fmt.Sprint(numSamples), 1)
	sliceSamples, _ = strconv.Atoi(poses[1])
	retrigs := []int{
		sliceSamples * 8,
		sliceSamples * 4,
		sliceSamples * 3,
		sliceSamples * 2,
		sliceSamples * 1,
		sliceSamples / 2,
		sliceSamples / 3,
	}
	retrigStrings := make([]string, len(retrigs))
	for i, v := range retrigs {
		retrigStrings[i] = fmt.Sprint(v)
	}
	ss = strings.Replace(ss, "WWW", strings.Join(retrigStrings, ","), 1)


	return
}
