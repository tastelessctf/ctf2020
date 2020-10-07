# Such Online

This challenge is essentially an online multiplayer maze.

However, it is impossible to solve the maze, as there is no way leading to the flag.

The JavaScipit client uses websockets to send position (or rather: coordinates where the player wants to go) to the server. The server sends an location and player update every 100ms.

When the client sends the "ToX" and "ToY" coordinates where it wants to go, the backend unmarshals those values in the player struct, and calls `Step`, to compute the next position when walking there.

However, as `ToX` and `ToY` are directly unmarshalled into the Player struct, it is possible to push direct `X` and `Y` coordinates.

Just set a breakpoint before the `setInterval` javascript, and send `ws.send(JSON.stringify({"X": 400,"Y":400}))` to beam to the center of the maze.
Once you hit the flag there the server will send a `flag` response packet, which you can see in the browsers web developer tools.