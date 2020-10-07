package main

import (
	"fmt"
	"net/http"
)

var d = "nothing to see here ntgvtVdBML9mh75WDAqggcDRexEYoG27XYWuUaixKomq8GGg7ZfRRg3ZGk5QyYMMTrUcwMhKqiETGZVN6E6VqfJcFcEfCfgrweBNuzf8qBDBGeaT2uMbRjVebPzWiCTc2jKy4UgwgkuUgtiBHuuZYGFqmFePedshPN6Zc3sRT8fdAEQLMAMHhcypB2Yb9sLy85VSsk7wHTVkCNNmEgSySh5SwfNc4eG65U7mHNL5DBPsSu3Pcua8PhQVXrx67KPKTKR1jrvPrCAuEeqvZLyaWcwQG1hGUnsN"

func main() {
	http.Handle("/rolling", http.RedirectHandler("https://www.youtube.com/watch?v=dQw4w9WgXcQ", http.StatusSeeOther))

	http.HandleFunc("/", func(rw http.ResponseWriter, r *http.Request) {
		if r.ProtoMajor == 1 && r.ProtoMinor == 0 {
			fmt.Fprintln(rw, "tstlss{http 1.0 is best http}")
			return
		}

		if pusher, ok := rw.(http.Pusher); ok {
			pusher.Push("/such-hidden-much-wow", &http.PushOptions{
				Method: "HEAD",
				Header: http.Header{
					"flag": []string{"tstlss{http2! what next?}"},
				},
			})
			fmt.Fprintln(rw, d)
		} else {
			rw.Header().Set("Trailer", "Flag")
			fmt.Fprintln(rw, d)
			rw.Header().Set("Flag", "tstlss{always keep looking}")
		}
	})

	http.ListenAndServeTLS(":10000", "server.crt", "server.key", nil)
}
