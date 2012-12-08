
# ellipsoid
def f(x, y, z)
  2.0 * (x - 10.0) * (x - 10.0) + 0.7 * (y - 4.0) * (y - 4.0) + (z - 3.0) * (z - 3.0) - 10000
end

# sin
def f(x, y)
  100.0 * Math.sin(x) - 43.0 - y
end

100.times do
  x = rand(201) - 100
  y = rand(201) - 100
  c = f(x, y) > 0.0 ? 1 : -1
  puts "#{x} #{y} #{c}"
end

