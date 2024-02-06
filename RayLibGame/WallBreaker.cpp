#include "WallBreaker.h"
#include "raylib.h"


//1 - Include deltaTime in all speed, movement calculations to make sure
//consistent speed is applied across devices |WORKING|

//2 - Develop a CircleToRoundRectangleCollision to detect the collision that happens
//with the corners of the bricks |Working|

//3 - Depending on where the collision happens with the bricks, come up with a direction
//that will affect the x and y of the ball's speed |Working|

//4 - Include an unlimited number of levels that are created procedurally.
//Each time you spawn the game, a new layout should be displayed |WORKING|

//5 - Have pickups such as life that fall from certain bricks upon impact.
//Collecting them by the player, will activate the reward. |Working|

//6 - Include a cheat.Such as a pickup that temporarily(for instance for the next 5 seconds)
//[with a time feedback for the user] eliminates the reversal of speed upon impact with bricks. |Not Working yet|

//I had some problems with the pickup movement, but I managed to make the pickups work .
//I'd grade myself with a 10/15

void WallBreaker::GenerateBricksForLevel() {
	// Clear existing bricks
	bricks.clear();
	pickups.clear();

	
	if (currentLevel < 1) 
	{
		currentLevel = 1;
	}
	else if (currentLevel > 100) {
		currentLevel = 100;
	}

	// Calculate maximum width and height for the triangle
	int maxWidth = BRICKS_PER_ROW;
	int maxHeight = (currentLevel > 7) ? 7 : currentLevel;  

	// Calculate brick size based on screen width and height
	float spaceForBricks = screenWidth - (maxWidth * GAP + GAP);
	Vector2 brickSize = Vector2{ spaceForBricks / maxWidth, BRICK_HEIGHT };

	// Generate the symmetrical triangle layout
	for (int row = 0; row < maxHeight; row++) {
		int numBricksInRow = (maxWidth > row + 1) ? row + 1 : maxWidth;  
		float xOffset = (maxWidth - numBricksInRow) * brickSize.x / 2;

		for (int col = 0; col < numBricksInRow; col++) {
			float x = GAP + xOffset + (GAP + brickSize.x) * col;
			float y = GAP + (GAP + brickSize.y) * row;

			Rectangle rect = Rectangle{ x, y, brickSize.x, brickSize.y };

			if (GetRandomValue(1, 100) <= 20) {
				float pickupX = rect.x + rect.width / 2;
				float pickupY = rect.y + rect.height / 2;
				Pickup pickup = { Vector2{pickupX, pickupY}, 10.0f, true };
				pickups.push_back(pickup);
			}
			

			Brick brick = Brick{ colors[row % ROWS_OF_BRICKS], rect };
			bricks.push_back(brick);
		}
	}
}

void WallBreaker::Main()
{
	InitWindow(screenWidth, screenHeight, "Wall Breaker");

	Start();

	SetTargetFPS(60); // This will become important
	while (!WindowShouldClose())
	{
		Update();
	}
	CloseWindow();
}

void WallBreaker::Start()
{
#pragma region BRICKS
	float spaceForBricks = screenWidth - (BRICKS_PER_ROW * GAP + GAP);
	Vector2 brickSize = Vector2{ spaceForBricks / BRICKS_PER_ROW, BRICK_HEIGHT };

	// Call GenerateBricksForLevel to create the initial layout
	GenerateBricksForLevel();
#pragma endregion 

#pragma region PLAYER
	player.position = Vector2{ screenWidth / 2, screenHeight * 9 / 10 };
	player.size = Vector2{ screenWidth / 10 , 20 };
	player.curLife = MAX_LIVES; // at the beginning
#pragma endregion

	ball.position = Vector2{ screenWidth / 2, screenHeight * 9 / 10 - 30 };
}


void WallBreaker::EvalCurFrame()
{
	if (gameOver)
	{
		if (IsKeyPressed(KEY_ENTER))
		{
			Start();
			gameOver = false;
		}

		return;
	}

	if (IsKeyPressed(KEY_P))
		gamePaused = !gamePaused;

	if (gamePaused) return;

	

	if (!ball.active)
	{
		ball.position = Vector2{ player.position.x, screenHeight * 9 / 10 - 30 };
		

		if (IsKeyPressed(KEY_SPACE))
		{
			ball.active = true;
			ball.speed = Vector2{ 0, -5 * ballSpeedMultiplier };
		}
	}
	else
	{
		
		ball.position.x += ball.speed.x * 15 * deltaTime;
		ball.position.y += ball.speed.y * deltaTime;
	}



	// player position
	if (IsKeyDown(KEY_LEFT))
		player.position.x -= 5;

	// we have reached the far left
	if (player.position.x - player.size.x / 2 <= 0)
		player.position.x = player.size.x / 2;

	// we have reached the far right
	if (player.position.x + player.size.x / 2 >= screenWidth)
		player.position.x = screenWidth - player.size.x / 2;

	if (IsKeyDown(KEY_RIGHT))
		player.position.x += 5;

	// collision with the walls
	if ((ball.position.x + ball.radius >= screenWidth) || ball.position.x - ball.radius <= 0)
		ball.speed.x *= -1;
	if (ball.position.y - ball.radius <= 0) // we hit the top
		ball.speed.y *= -1;
	if (ball.position.y + ball.radius >= screenHeight)
	{
		player.curLife--;
		ball.active = false;
		ball.speed = Vector2{ 0 , 0 };
	}

	// Check for collision with bricks
	for (int i = 0; i < bricks.size(); i++)
	{
		if (CheckCollisionCircleRec(ball.position, ball.radius, bricks[i].rect))
		{

			// reverse the direction of the ball
			ball.speed.y *= -1;

			if (ball.position.x < bricks[i].rect.x) {

				// Ball hits the left corner, move left
				ball.speed.x = -10 * deltaTime; 
			}
			else if (ball.position.x > bricks[i].rect.x + bricks[i].rect.width) {
				// Ball hits the right corner, move right
				ball.speed.x = 10 * deltaTime;
			}
			else {
				// Ball hits the top or bottom corner, reverse x-speed
				ball.speed.x *= -1;
			}
			// delete the brick
			bricks.erase(bricks.begin() + i);
			
			

			break; // Because the ball might hit two bricks at the same frame		
		}
	}

	// Check for collision with pickups
	for (int i = 0; i < pickups.size(); i++) {
		if (CheckCollisionCircleRec(ball.position, ball.radius, Rectangle{ pickups[i].position.x, pickups[i].position.y, 1, 1 })) {
			if (pickups[i].active)
			{
				pickups[i].position.y++;
			}
			pickups[i].active = false;

			player.curLife++;

			break;
		}
	}



	// Colision with the pedal
	if (CheckCollisionCircleRec(ball.position, ball.radius, player.GetRect()))
	{
		if (ball.speed.y > 0) // we are going downwards
		{
			ball.speed.y *= -1;
			ball.speed.x = (ball.position.x - player.position.x) / (player.size.x / 10);
		}
	}

	


	if (player.curLife == 0)
		gameOver = true;
	else
	{
		if (bricks.size() == 0)
			levelWon = true;
	}


}


void WallBreaker::DrawCurFrame()
{
	BeginDrawing();
	ClearBackground(BLACK);

	if (gameOver)
	{
		DrawText("Press Enter to play again",
			GetScreenWidth() / 2 - MeasureText("Press Enter to play again", 30) / 2,
			GetScreenHeight() / 2 - 15,
			30, GRAY);
	}
	else if (levelWon)
	{
		DrawText("You Won! Press Enter to go to next level!",
			GetScreenWidth() / 2 - MeasureText("You Won! Press Enter to go to next level!", 30) / 2,
			GetScreenHeight() / 2 - 15,
			30, GRAY);
	}
	else
	{
		player.Draw();
		ball.Draw();

		
		// Draw pickups
		for (const Pickup& pickup : pickups) {
			if (pickup.active) {
				pickup.Draw();
			}
		}


		for (Brick b : bricks)
			b.Draw();


		// draw player lives!
		for (int i = 0; i < MAX_LIVES; i++)
		{
			if (i < player.curLife)
				DrawRectangle(10 + 40 * i, screenHeight - 20, 30, 10, LIGHTGRAY);
			else
				DrawRectangle(10 + 40 * i, screenHeight - 20, 30, 10, GRAY);
		}
	}





	EndDrawing();
}

void WallBreaker::Update()
{
	deltaTime = GetFrameTime();
	EvalCurFrame();
	DrawCurFrame();
	
	if (levelWon) {
		levelWon = false;
		currentLevel++;
		Start();
	}

}

