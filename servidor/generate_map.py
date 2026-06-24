import random

MAP_SIZE = 40
maze = [['0' for _ in range(MAP_SIZE)] for _ in range(MAP_SIZE)]

# Create a border
for i in range(MAP_SIZE):
    maze[0][i] = 'X'
    maze[MAP_SIZE-1][i] = 'X'
    maze[i][0] = 'X'
    maze[i][MAP_SIZE-1] = 'X'

# Create a simple maze pattern (grid of blocks)
for i in range(2, MAP_SIZE-2, 2):
    for j in range(2, MAP_SIZE-2, 2):
        maze[i][j] = 'X'
        if random.random() > 0.5:
            maze[i][j-1] = 'X'
        else:
            maze[i-1][j] = 'X'

# Define artifacts
artifacts = ['P', '1', '2', '3', '4', '5', '6', 'R', 'B', 'G', 'Y']

# Place artifacts
for item in artifacts:
    while True:
        x = random.randint(1, MAP_SIZE-2)
        y = random.randint(1, MAP_SIZE-2)
        if maze[x][y] == '0':
            maze[x][y] = item
            break

# Write to file
with open('/home/exati/projetos/projeto-redes-pacman/servidor/mapa.csv', 'w') as f:
    line = []
    for i in range(MAP_SIZE):
        for j in range(MAP_SIZE):
            line.append(maze[i][j])
    f.write(';'.join(line) + ';')

print("mapa.csv generated successfully")
