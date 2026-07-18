local Player = { name = "default", hp = 50, mp = 2 }
Player.__index = Player
Player.__newindex = function() error(" DONT DO THAT" , 3) end

function Player.new(name) 
  local instance = { name = name, hp = 100, mp = 45 }
  instance.crazy = 8
  setmetatable( instance, Player )
  return instance
end

function Player:takeDamage(amount)
  self.hp = self.hp - amount
  print(self.name .. " took damage! HP is now: " .. self.hp)
end

local hero = Player.new("Arthur")
local goblin = Player.new("Goblin")

hero:takeDamage(30)    
goblin:takeDamage(35)    
hero:takeDamage(3)    
goblin:takeDamage(5)    

local abc = { cuteness = 5000 }
setmetatable( abc, Player )
print("crazy is "..tostring(abc.cuteness))
abc:takeDamage(8)
