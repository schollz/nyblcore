package main

import (
	"fmt"
	"io"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"strconv"
	"strings"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/youpy/go-wav"
)

var Version = "v0.5.0"

func main() {
	router := gin.Default()
	// Set a lower memory limit for multipart forms (default is 32 MiB)
	router.MaxMultipartMemory = 8 << 20 // 8 MiB
	router.LoadHTMLGlob("template/*")
	router.GET("/", func(c *gin.Context) {
		c.HTML(http.StatusOK, "index.html", gin.H{
			"version": Version,
		})

	})
	router.POST("/", func(c *gin.Context) {
		var err error
		var split_files []string

		defer func() {
			if err != nil {
				c.HTML(http.StatusOK, "index.html", gin.H{
					"version": Version,
					"error":   err.Error(),
				})

			}
			for _, f := range split_files {
				os.Remove(f)
			}
		}()
		slicesString := c.PostForm("slices")
		slices, _ := strconv.Atoi(slicesString)
		crossfadeString := c.PostForm("crossfade")
		crossfade, _ := strconv.Atoi(crossfadeString)
		crossfadeTimeString := fmt.Sprintf("%2.6f",float64(crossfade)/1000.0)

		// Multipart form
		form, err := c.MultipartForm()
		if err != nil {
			return
		}
		files := form.File["files"]

		var cmd1 []string
		for _, file := range files {
			filename := filepath.Base(file.Filename)
			if strings.Contains(filename, ".wav") || strings.Contains(filename, ".mp3") || strings.Contains(filename, ".flac") || strings.Contains(filename, ".aif") || strings.Contains(filename, ".ogg") {
				cmd1 = append(cmd1, filename)
				if err := c.SaveUploadedFile(file, filename); err != nil {
					c.String(http.StatusBadRequest, "upload file err: %s", err.Error())
					return
				}
			}
		}

		filenames := cmd1
		if slices == 0 {
			slices = len(cmd1)
		}
		cmd1 = append(cmd1, []string{"/tmp/0.wav"}...)
		cmd := exec.Command("sox", cmd1...)
		output, err := cmd.CombinedOutput()
		if err != nil {
			return
		}
		fmt.Println("sox", cmd1)

		cmd2 := []string{"/tmp/0.wav", "-r", "4400", "-c", "1", "-b", "8", "/tmp/1.wav", "norm", "lowpass", "2200", "trim", "0", "1.2", "dither"}
		cmd = exec.Command("sox", cmd2...)
		output, err = cmd.CombinedOutput()
		if err != nil {
			return
		}
		fmt.Println("sox", cmd2)
		fmt.Println(string(output))

		// split the file
		samples, err := numSamples("/tmp/1.wav")
		if err != nil {
			return
		}
		cmd3 := []string{"/tmp/1.wav", "/tmp/ready.wav", "trim", "0", fmt.Sprintf("%ds", samples/slices), ":", "newfile", ":", "restart"}
		if crossfade>0 {
			cmd3 = []string{"/tmp/1.wav", "/tmp/ready.wav", "trim", "0", fmt.Sprintf("%ds", samples/slices),"fade","h",crossfadeTimeString, fmt.Sprintf("%ds", samples/slices),crossfadeTimeString,":", "newfile", ":", "restart"}
		}
		fmt.Println(cmd3)
		cmd = exec.Command("sox", cmd3...)
		output, err = cmd.CombinedOutput()
		if err != nil {
			err = fmt.Errorf("could not split file: %s", err.Error())
			return
		}
		split_files, err = filepath.Glob("/tmp/ready*wav")
		if err != nil {
			return
		}
		fmt.Println(split_files)

		bs, err := os.ReadFile("../nyblcore/nyblcore.ino")
		if err != nil {
			return
		}

		converted := string(bs)

		data, err := convertWavToInts(split_files)
		if err != nil {
			return
		}

		converted = strings.Replace(converted, "// SAMPLETABLE", data, 1)

		converted = `// ` + fmt.Sprintf("nyblcore v0.5 generated %s\n", time.Now()) +
			`// ` + fmt.Sprintf("%s\n", strings.Join(filenames, " ")) +
			`// ` + fmt.Sprintf("%d slices\n\n\n", slices) +
			converted

		c.HTML(http.StatusOK, "index.html", gin.H{
			"version": Version,
			"code":    converted,
		})
	})
	router.Run(":8080")
}

func numSamples(fname string) (samples int, err error) {
	r, _ := regexp.Compile(`(\d+) samples`)
	r2, _ := regexp.Compile(`(\d+)`)
	cmd := exec.Command("sox", "--i", fname)
	output, err := cmd.CombinedOutput()
	if err != nil {
		return
	}
	fmt.Println(string(output))
	match := r.FindString(string(output))
	match2 := r2.FindString(match)
	samples, err = strconv.Atoi(match2)

	return
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
					sampleString += ","
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
