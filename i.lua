Stats = {}
statsMt = { 
	__tostring = function(x)
		local output = ""
		for k, v in pairs(x) do
			output = output .. "\n" .. tostring(k) .. ": " .. tostring(v)
		end
		return output
	end,

	__add = function(x, y)
		local output = Stats.new()
		for k, v in pairs(x) do
			output[k] = v + y[k]
		end
		return output
	end, 

	__sub = function(x, y)
		local output = Stats.new()
		for k, v in pairs(x) do
			output[k] = v - y[k]
		end
		return output
	end,

	__newindex = function(k, v) error( "Can't assign key " .. k .. " to Stat object.", 2) end
}

function Stats.new(T)
  local defaults = {
    hp = 0,
    mp = 0,
    strength = 0, 
    speed = 0,
    iq = 0
  }

  local t = T or {}

  for k, v in pairs(defaults) do
    t[k] = t[k] or v
  end

  setmetatable( t, statsMt )

  return t
end

bill = Stats.new( { hp = 8} )
bob = Stats.new()
jean = Stats.new()
potion = Stats.new( {hp = 100} )

bill = bill + potion
print("\nbill's final state:")
print(bill)
bill = bill - Stats.new( {hp = 90} )
print("\nbill's final state:")
print(bill)

-- t = gets(45)
-- print( "hp is " .. t.hp .. ", mp is " .. t.mp .. ", speed is " .. t.speed )

sys( System.TIMER, Action.STOP, 1 )
for k, v in pairs(Component) do print(k) end
c = getComponent( Component.RECT, 1 )
print( "c.x is " .. c.x )
