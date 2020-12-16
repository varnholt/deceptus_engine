------------------------------------------------------------------------------------------------------------------------
require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"


------------------------------------------------------------------------------------------------------------------------
properties = {
   staticBody = false,
   sprite = "data/sprites/enemy_spikeball.png",
   damage = 200
}


------------------------------------------------------------------------------------------------------------------------
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mSpriteIndex = 0


-- spike ball concept
--
--                      +-----------+
--                      |           |
--                      |     x     |    box body + rotating revolute joint (static body)
--                      |   ./      |
--                      +-./--------+
--                      ./               thin box body + distance joint      _________   _________   _________
--                    ./                 thin box body + distance joint    -[-o     o-]-[-o     o-]-[-o     o-]-
--                  ./                   thin box body + distance joint     '---------' '---------' '---------'
--            \- __^_ -/
--             .`    '.
--           < : O  o : >                circular body (bad spiky ball, dynamic body)
--             :  __  :
--            /_`----'_\
--                \/
--
-- https://www.iforce2d.net/b2dtut/joints-revolute

------------------------------------------------------------------------------------------------------------------------
function initialize()

   addShapeRect(0.2, 0.2, -0.05, 0.0)
   updateSpriteRect(0, 0, 0, 48, 48)
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
end


------------------------------------------------------------------------------------------------------------------------
function setPath(name, table)
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   mPosition = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
   mPlayerPosition = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
end
