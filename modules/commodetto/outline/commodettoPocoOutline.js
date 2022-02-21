import Poco from "commodetto/Poco";

function blendOutline(color, blend, outline, x, y) @ "xs_outlinerenderer_blendOutline";
function blendPolygon(color, blend, polygon) @ "xs_outlinerenderer_blendPolygon";
Poco.prototype.blendOutline = blendOutline
Poco.prototype.blendPolygon = blendPolygon;

export default Object.freeze({});

