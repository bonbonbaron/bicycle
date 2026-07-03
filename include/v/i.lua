Character = {
  new = function(name)
    instance = {}
    instance.name = name
    instance.hp = 100
    instance.mp = 100
    instance.level = 1
    instance.attack = 10
    return instance
  end
}

c = Character.new("Bill")
d = Character.new("Carl")
e = d.new("Jarl")

all = { c, d, e }

for _, cc in ipairs(all) do
  print( cc.name.."'s".." hp: "..cc.hp )
end

