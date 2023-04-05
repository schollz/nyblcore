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
	log "github.com/schollz/logger"
	"github.com/youpy/go-wav"
)

var Version = "v0.5.0"

func main() {
	log.SetLevel("trace")

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
				log.Tracef("removing %s", f)
				os.Remove(f)
			}
		}()
		slicesString := c.PostForm("slices")
		slices, _ := strconv.Atoi(slicesString)
		crossfadeString := c.PostForm("crossfade")
		crossfade, _ := strconv.Atoi(crossfadeString)
		crossfadeTimeString := fmt.Sprintf("%2.6f", float64(crossfade)/1000.0)

		// Multipart form
		form, err := c.MultipartForm()
		if err != nil {
			log.Error(err)
			return
		}

		files := form.File["files"]
		var filenames []string
		var originalFilenames []string
		for _, file := range files {
			filename := filepath.Base(file.Filename)
			if strings.Contains(filename, ".wav") || strings.Contains(filename, ".flac") || strings.Contains(filename, ".aif") || strings.Contains(filename, ".ogg") {
				// open a temp file to save the uploaded file
				ftemp, err := os.CreateTemp("", "nyblcore*"+filepath.Ext(filename))
				if err != nil {
					log.Error(err)
					return
				}
				ftemp.Close()
				defer os.Remove(ftemp.Name())

				// save the uploaded file
				if err := c.SaveUploadedFile(file, ftemp.Name()); err != nil {
					log.Error(err)
					return
				}

				// open a tempfile to save the resampled version
				ftemp2, err := os.CreateTemp("", "nyblcore*.wav")
				if err != nil {
					log.Error(err)
					return
				}
				defer os.Remove(ftemp2.Name())

				// convert the incoming file to lower sample rate
				cmdString := []string{"sox", ftemp.Name(), "-r", "4400", "-c", "1", "-b", "8", ftemp2.Name(), "norm", "lowpass", "2200", "trim", "0", "1.2", "dither"}
				if len(files) > 1 && crossfade > 0 {
					cmdString = []string{"sox", ftemp.Name(), "-r", "4400", "-c", "1", "-b", "8", ftemp2.Name(), "norm", "lowpass", "2200", "trim", "0", "1.2", "dither", "fade", "h", crossfadeTimeString, "-0", crossfadeTimeString}
				}
				cmd := exec.Command(cmdString[0], cmdString[1:]...)
				var output []byte
				output, err = cmd.CombinedOutput()
				if err != nil {
					err = fmt.Errorf("error: '%s'\n\n%s", err.Error(), output)
					log.Error(err)
					return
				}
				log.Tracef("%s", output)

				// append it to the list
				log.Tracef("adding '%s' ('%s') to the list", ftemp2.Name(), file.Filename)
				filenames = append(filenames, ftemp2.Name())
				originalFilenames = append(originalFilenames, filename)
			}
		}

		if len(filenames) == 0 {
			err = fmt.Errorf("must choose at least one audio file")
			return
		}

		// take action based on the number of input files
		if len(filenames) == 1 {
			// split the file if its a single file
			samples, err := numSamples(filenames[0])
			if err != nil {
				return
			}

			// create a tempfile to store it
			ftemp, err := os.CreateTemp("", "nyblcore_split*.wav")
			if err != nil {
				return
			}
			ftemp.Close()
			os.Remove(ftemp.Name())

			cmd3 := []string{filenames[0], ftemp.Name(), "trim", "0", fmt.Sprintf("%ds", samples/slices), ":", "newfile", ":", "restart"}
			if crossfade > 0 {
				cmd3 = []string{filenames[0], ftemp.Name(), "trim", "0", fmt.Sprintf("%ds", samples/slices), "fade", "h", crossfadeTimeString, fmt.Sprintf("%ds", samples/slices), crossfadeTimeString, ":", "newfile", ":", "restart"}
			}
			log.Trace(cmd3)
			cmd := exec.Command("sox", cmd3...)
			var output []byte
			output, err = cmd.CombinedOutput()
			if err != nil {
				err = fmt.Errorf("could not split file: %s\n\n%s", err.Error(), output)
				return
			}
			split_files, err = filepath.Glob(strings.Replace(ftemp.Name(), ".wav", "*", -1))
			if err != nil {
				return
			}
		} else {
			split_files = filenames
		}

		log.Tracef("split_files: %+v", split_files)

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
			`// ` + fmt.Sprintf("%s\n", strings.Join(originalFilenames, " ")) +
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
	log.Trace(string(output))
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
