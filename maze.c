#include "maze.h"

int worldMap[MAP_WIDTH][MAP_HEIGHT];

double posX = 10, posY = 4;
double dirX = -1, dirY = 0;
double planeX = 0, planeY = 0.66;

int showMap = 1;

SDL_Texture *textures[2];

void render(SDL_Renderer *renderer) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        double cameraX = 2 * x / (double)SCREEN_WIDTH - 1;
        double rayDirX = dirX + planeX * cameraX;
        double rayDirY = dirX * cameraX;

        int mapX = (int)posX;
        int mapY = (int)posY;

        double sideDistX;
        double sideDistY;

        double deltaDistX = fabs(1 / rayDirX);
        double deltaDistY = fabs(1 / rayDirY);
        double perpWallDist;

        int stepX;
        int stepY;

        int hit = 0;
        int side;

        if (rayDirX < 0) {
            stepX = -1;
            sideDistX = (posX - mapX) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (mapX + 1.0 - posX) * deltaDistX;
        }
        if (rayDirY < 0) {
            stepY = -1;
            sideDistY = (posY - mapY) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (mapY + 1.0 - posY) * deltaDistY;
        }

        while (hit == 0) {
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }
            if (worldMap[mapX][mapY] > 0) hit = 1;
        }

        if (side == 0) perpWallDist = (mapX - posX + (1 - stepX) / 2) / rayDirX;
        else perpWallDist = (mapY - posY + (1 - stepY) / 2) / rayDirY;

        int lineHeight = (int)(SCREEN_HEIGHT / perpWallDist);

        int drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2;
        if (drawStart < 0) drawStart = 0;
        int drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2;
        if (drawEnd >= SCREEN_HEIGHT) drawEnd = SCREEN_HEIGHT - 1;

	SDL_Texture *texture = textures[0];
        if (side == 1) {
            texture = textures[1];
        }

	double wallX;
        if (side == 0) wallX = posY + perpWallDist * rayDirY;
        else wallX = posX + perpWallDist * rayDirX;
        wallX -= floor(wallX);

	int texX = (int)(wallX * 64);
        if (side == 0 && rayDirX > 0) texX = 64 - texX - 1;
        if (side == 1 && rayDirY < 0) texX = 64 - texX - 1;

	for (int y = drawStart; y < drawEnd; y++) {
            int d = y * 256 - SCREEN_HEIGHT * 128 + lineHeight * 128;
            int texY = ((d * 64) / lineHeight) / 256;
            SDL_Rect srcRect = {texX, texY, 1, 1};
            SDL_Rect dstRect = {x, y, 1, 1};
            SDL_RenderCopy(renderer, texture, &srcRect, &dstRect);
        }
    }
}

void drawMiniMap(SDL_Renderer *renderer) {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (worldMap[y][x] > 0) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            }
            SDL_Rect rect = {x * MINI_MAP_SCALE, y * MINI_MAP_SCALE, MINI_MAP_SCALE, MINI_MAP_SCALE};
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_Rect playerRect = {(int)(posX * MINI_MAP_SCALE), (int)(posY * MINI_MAP_SCALE), MINI_MAP_SCALE, MINI_MAP_SCALE};
    SDL_RenderFillRect(renderer, &playerRect);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderDrawLine(renderer,
                       posX * MINI_MAP_SCALE + MINI_MAP_SCALE / 2,
                       posY * MINI_MAP_SCALE + MINI_MAP_SCALE / 2,
                       (posX + dirX) * MINI_MAP_SCALE + MINI_MAP_SCALE / 2,
                       (posY + dirY) * MINI_MAP_SCALE + MINI_MAP_SCALE / 2);
}

void rotateCamera(double angle) {
    double oldDirX = dirX;
    dirX = dirX * cos(angle) - dirY * sin(angle);
    dirY = oldDirX * sin(angle) + dirY * cos(angle);
    double oldPlaneX = planeX;
    planeX = planeX * cos(angle) - planeY * sin(angle);
    planeY = oldPlaneX * sin(angle) + planeY * cos(angle);
}

void moveCamera(double moveSpeed, double strafeSpeed) {
    if (worldMap[(int)(posX + dirX * moveSpeed)][(int)posY] == 0) posX += dirX * moveSpeed;
    if (worldMap[(int)posX][(int)(posY + dirY * moveSpeed)] == 0) posY += dirY * moveSpeed;
    if (worldMap[(int)(posX + planeX * strafeSpeed)][(int)posY] == 0) posX += planeX * strafeSpeed;
    if (worldMap[(int)posX][(int)(posY + planeY * strafeSpeed)] == 0) posY += planeY * strafeSpeed;
}

int parseMapFile(const char *filePath) {
    FILE *file = fopen(filePath, "r");
    if (file == NULL) {
        printf("Failed to open map file!\n");
        return -1;
    }

    char line[MAP_WIDTH + 2];
    int row = 0;

    while (fgets(line, sizeof(line), file) && row < MAP_HEIGHT) {
        for (int col = 0; col < MAP_WIDTH; col++) {
            if (line[col] == '1') {
                worldMap[row][col] = 1;
            } else if (line[col] == '0') {
                worldMap[row][col] = 0;
            }
        }
        row++;
    }

    fclose(file);
    return 0;
}

SDL_Texture* loadTexture(SDL_Renderer *renderer, const char *filePath) {
    SDL_Surface *surface = IMG_Load(filePath);
    if (!surface) {
        printf("Failed to load texture %s! IMG_Error: %s\n", filePath, IMG_GetError());
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    return texture;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <map_file_path>\n", argv[0]);
        return -1;
    }

    const char *mapFilePath = argv[1];

    if (parseMapFile(mapFilePath) != 0) {
        return -1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }


    SDL_Window *window = SDL_CreateWindow("Raycaster", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    textures[0] = loadTexture(renderer, "textures/redbrick.png");
    textures[1] = loadTexture(renderer, "textures/greystone.png");
    
    int quit = 0;
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_LEFT:
                        rotateCamera(-ROT_SPEED);
                        break;
                    case SDLK_RIGHT:
                        rotateCamera(ROT_SPEED);
                        break;
                    case SDLK_w:
                        moveCamera(MOVE_SPEED, 0);
                        break;
                    case SDLK_s:
                        moveCamera(-MOVE_SPEED, 0);
                        break;
                    case SDLK_a:
                        moveCamera(0, -MOVE_SPEED);
                        break;
                    case SDLK_d:
                        moveCamera(0, MOVE_SPEED);
                        break;
                    case SDLK_m:
                        showMap = !showMap;
                        break;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0, 128, 0, 255);
        SDL_Rect groundRect = {0, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT / 2};
        SDL_RenderFillRect(renderer, &groundRect);

        render(renderer);

        if (showMap) {
            drawMiniMap(renderer);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(textures[0]);
    SDL_DestroyTexture(textures[1]);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

