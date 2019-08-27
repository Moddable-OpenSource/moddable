let map = Compartment.map;
delete map.increment;
let mod = new Compartment("mod", {}, map);
