-- Copyright (C) 2022  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

do
  local spr = Sprite(4, 4, ColorMode.INDEXED)

  app.command.NewLayer{ tilemap=true }
  local tilemap = spr.layers[2]

  local tileset = tilemap.tileset
  assert(#tileset == 1);

  app.useTool{
    tool='pencil',
    color=1,
    layer=tilemap,
    tilesetMode=TilesetMode.STACK,
    points={ Point(0, 0) }}
  assert(#tileset == 2)

  assert(tileset.name == "")
  tileset.name = "Default Land"
  assert(tileset.name == "Default Land")

  -- Tileset user data
  assert(tileset.data == "")
  tileset.data = "land"
  assert(tileset.data == "land")

  assert(tileset.color == Color())
  tileset.color = Color(255, 0, 0)
  assert(tileset.color == Color(255, 0, 0))

  -- Create extra tile
  app.useTool{
    tool='pencil',
    color=1,
    layer=tilemap,
    tilesetMode=TilesetMode.STACK,
    points={ Point(1, 1) }}
  assert(#tileset == 3)

  -- Check that Tileset:getTile(ti) returns Tileset:tile(ti).image user data
  for ti=0,2 do
    assert(tileset:tile(ti).image.id == tileset:getTile(ti).id)
  end
end

do
  -- Check Sprite:newTile() and Sprite:deleteTile() methods
  local spr = Sprite(4, 4, ColorMode.INDEXED)
  local pal = spr.palettes[1]
  pal:setColor(0, Color(0, 0, 0))
  pal:setColor(1, Color(255, 255, 0))
  pal:setColor(2, Color(255, 0, 0))
  spr.gridBounds = Rectangle(0, 0, 2, 2)
  app.command.NewLayer{ tilemap=true }
  local tilemap = spr.layers[2]
  local tileset = tilemap.tileset
  assert(tileset.grid.tileSize == Size(2, 2))
  app.useTool{ tool="pencil",
               color=1,
               layer=tilemap,
               tilesetMode=TilesetMode.AUTO,
               tilemapMode=TilemapMode.PIXELS,
               points={Point(0,0), Point(1, 0)}}
  app.useTool{ tool="pencil",
               color=2,
               layer=tilemap,
               tilesetMode=TilesetMode.AUTO,
               tilemapMode=TilemapMode.PIXELS,
               points={Point(2,0), Point(3, 1)}}
  assert(#tileset == 3)
  -- Insert a tile in the middle of the tileset
  local tile1 = spr:newTile(tileset, 1)
  assert(#tileset == 4)
  expect_img(tileset:getTile(0), { 0, 0,
                                   0, 0 })
  expect_img(tileset:getTile(1), { 0, 0,
                                   0, 0 })
  expect_img(tileset:getTile(2), { 1, 1,
                                   0, 0 })
  expect_img(tileset:getTile(3), { 2, 0,
                                   0, 2 })
  
  app.undo()
  
  assert(#tileset == 3)
  expect_img(tileset:getTile(0), { 0, 0,
                                   0, 0 })
  expect_img(tileset:getTile(1), { 1, 1,
                                   0, 0 })
  expect_img(tileset:getTile(2), { 2, 0,
                                   0, 2 })
  
  app.redo()

  assert(#tileset == 4)
  expect_img(tileset:getTile(0), { 0, 0,
                                   0, 0 })
  expect_img(tileset:getTile(1), { 0, 0,
                                   0, 0 })
  expect_img(tileset:getTile(2), { 1, 1,
                                   0, 0 })
  expect_img(tileset:getTile(3), { 2, 0,
                                   0, 2 })
  -- Insert a tile at the end of the tileset (by index omission)
  local tile2 = spr:newTile(tileset)
  assert(#tileset == 5)
  expect_img(tileset:getTile(3), { 2, 0,
                                   0, 2 })
  expect_img(tileset:getTile(4), { 0, 0,
                                   0, 0 })
  -- Insert at the end of the tileset explicity, and testing
  -- other way to access to the tile image
  local tile3 = spr:newTile(tileset, 5)
  assert(#tileset == 6)
  expect_img(tileset:tile(3).image, { 2, 0,
                                      0, 2 })
  expect_img(tileset:tile(4).image, { 0, 0,
                                      0, 0 })
  expect_img(tileset:tile(5).image, { 0, 0,
                                      0, 0 })

  app.undo()
  app.undo()
  app.undo()

  assert(#tileset == 3)
  expect_img(tileset:getTile(0), { 0, 0,
                                   0, 0 })
  expect_img(tileset:getTile(1), { 1, 1,
                                   0, 0 })
  expect_img(tileset:getTile(2), { 2, 0,
                                   0, 2 })
  
  app.redo()
  app.redo()
  app.redo()

  assert(#tileset == 6)
  expect_img(tileset:getTile(0), { 0, 0,
                                   0, 0 })
  expect_img(tileset:getTile(1), { 0, 0,
                                   0, 0 })
  expect_img(tileset:getTile(2), { 1, 1,
                                   0, 0 })
  expect_img(tileset:getTile(3), { 2, 0,
                                   0, 2 })
  expect_img(tileset:getTile(4), { 0, 0,
                                   0, 0 })
  expect_img(tileset:getTile(5), { 0, 0,
                                   0, 0 })
  -- Deleting tiles
  spr:deleteTile(tileset:tile(5))
  spr:deleteTile(tileset:tile(4))
  spr:deleteTile(tileset:tile(1))

  app.undo()

  assert(#tileset == 4)
  expect_img(tileset:getTile(0), { 0, 0,
                                   0, 0 })
  expect_img(tileset:getTile(1), { 0, 0,
                                   0, 0 })
  expect_img(tileset:getTile(2), { 1, 1,
                                   0, 0 })
  expect_img(tileset:getTile(3), { 2, 0,
                                   0, 2 })
  
  app.redo()

  assert(#tileset == 3)
  expect_img(tileset:getTile(0), { 0, 0,
                                   0, 0 })
  expect_img(tileset:getTile(1), { 1, 1,
                                   0, 0 })
  expect_img(tileset:getTile(2), { 2, 0,
                                   0, 2 })
  spr:deleteTile(tileset:tile(1))
  assert(#tileset == 2)
  expect_img(tileset:getTile(0), { 0, 0,
                                   0, 0 })
  expect_img(tileset:getTile(1), { 2, 0,
                                   0, 2 })
  
  app.undo()
  app.undo()
  app.undo()
  app.undo()
  
  assert(#tileset == 6)
  expect_img(tileset:getTile(0), { 0, 0,
                                   0, 0 })
  expect_img(tileset:getTile(1), { 0, 0,
                                   0, 0 })
  expect_img(tileset:getTile(2), { 1, 1,
                                   0, 0 })
  expect_img(tileset:getTile(3), { 2, 0,
                                   0, 2 })
  expect_img(tileset:getTile(4), { 0, 0,
                                   0, 0 })
  expect_img(tileset:getTile(5), { 0, 0,
                                   0, 0 })
  
end