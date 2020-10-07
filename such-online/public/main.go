package main

import (
	"image"
	"image/png"
	"log"
	"math/rand"
	"net/http"
	"os"
	"sync"
	"time"

	"github.com/goombaio/namegenerator"
	"github.com/gorilla/websocket"
)

var level [800][800]int8

func getImageFromFilePath(filePath string) (image.Image, error) {
	f, err := os.Open(filePath)
	if err != nil {
		return nil, err
	}
	image, err := png.Decode(f)
	return image, err
}

func init() {
	img, err := getImageFromFilePath("maze.png")
	if err != nil {
		log.Fatal(err)
	}

	for i := 0; i < 800; i++ {
		level[0][i] = 1
		level[i][0] = 1
		level[799][i] = 1
		level[i][799] = 1
		level[798][i] = 1
		level[i][798] = 1
	}

	for i := 1; i < 798; i++ {
		for j := 1; j < 798; j++ {
			r, g, b, _ := img.At(i, j).RGBA()
			if r > 0 || g > 0 || b > 0 {
				level[i][j] = 1
			}
		}
	}

	for i := 380; i < 420; i++ {
		for j := 380; j < 420; j++ {
			level[i][j] = 2
		}
	}
}

type Player struct {
	Name     string
	X, Y     int
	ToX, ToY int
}

func (player *Player) Step() bool {
	if player.X < 0 {
		player.X = 0
	}
	if player.Y < 0 {
		player.Y = 0
	}
	if player.ToX < 0 {
		player.ToX = 0
	}
	if player.ToY < 0 {
		player.ToY = 0
	}

	if player.X > 799 {
		player.X = 799
	}
	if player.Y > 799 {
		player.Y = 799
	}
	if player.ToX > 799 {
		player.ToX = 799
	}
	if player.ToY > 799 {
		player.ToY = 799
	}

	for i := 0; i < 5; i++ {
		if player.ToX > player.X {
			player.X++
			if level[player.X][player.Y] == 0 {
				player.X--
			}
		} else if player.ToX < player.X {
			player.X--
			if level[player.X][player.Y] == 0 {
				player.X++
			}
		}
		if player.ToY > player.Y {
			player.Y++
			if level[player.X][player.Y] == 0 {
				player.Y--
			}
		} else if player.ToY < player.Y {
			player.Y--
			if level[player.X][player.Y] == 0 {
				player.Y++
			}
		}
	}

	return level[player.X][player.Y] == 2
}

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool { return true },
}

type client struct {
	player *Player
	ws     *websocket.Conn
	dirty  bool
}

var clients = make(map[string]*client)
var cm = new(sync.RWMutex)

var seed = time.Now().UTC().UnixNano()
var nameGenerator = namegenerator.NewNameGenerator(seed)

func ws(w http.ResponseWriter, r *http.Request) {
	c, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Print("upgrade:", err)
		return
	}
	defer c.Close()

	var player Player

	name := nameGenerator.Generate()
	cm.RLock()
	for {
		if _, ok := clients[name]; ok {
			name = nameGenerator.Generate()
		} else {
			break
		}
	}
	cm.RUnlock()
	player.Name = name

	log.Println("new player: ", player.Name)

	c.WriteJSON(player)

	cm.Lock()
	clients[name] = &client{
		ws:     c,
		player: &player,
		dirty:  true,
	}
	cm.Unlock()

	switch rand.Intn(4) {
	case 0:
		player.X = 1
		player.Y = 1 + rand.Intn(798)
	case 1:
		player.X = 799
		player.Y = 1 + rand.Intn(798)
	case 2:
		player.Y = 1
		player.X = 1 + rand.Intn(798)
	case 3:
		player.Y = 799
		player.X = 1 + rand.Intn(798)
	}

	timer := time.AfterFunc(10*time.Second, func() {
		c.Close()
	})

	for {
		err := c.ReadJSON(&player)
		if err != nil {
			log.Println("read:", err)
			break
		}

		timer.Reset(10 * time.Second)

		player.Name = name

		oldx, oldy := player.X, player.Y

		if player.Step() {
			cm.Lock()
			c.WriteJSON(map[string]interface{}{"flag": os.Getenv("FLAG")})
			cm.Unlock()
		}

		if oldx != player.X || oldy != player.Y {
			cm.Lock()
			clients[name].dirty = true
			cm.Unlock()
		}
	}
}

func main() {
	go func() {
		for range time.Tick(100 * time.Millisecond) {
			cm.Lock()
			updates := make([]Player, 0, 100)
			for _, c := range clients {
				if c.dirty {
					updates = append(updates, Player{Name: c.player.Name, X: c.player.X, Y: c.player.Y})
					c.dirty = false
				}
			}
			if len(updates) > 0 {
				log.Printf("%d updates to %d players", len(updates), len(clients))
				for n, c := range clients {
					if err := c.ws.WriteJSON(updates); err != nil {
						delete(clients, n)
					}
				}
			}
			cm.Unlock()
		}
	}()

	http.HandleFunc("/", func(rw http.ResponseWriter, r *http.Request) {
		http.ServeFile(rw, r, "index.html")
	})
	http.HandleFunc("/maze.png", func(rw http.ResponseWriter, r *http.Request) {
		http.ServeFile(rw, r, "maze.png")
	})
	http.HandleFunc("/ws", ws)
	server := &http.Server{Addr: ":12345", Handler: nil}
	server.ListenAndServe()
}
