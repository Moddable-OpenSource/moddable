#include "commodettoPoco.h"

struct PocoOutlineRecord {
	uint16_t	n_points;
	uint16_t	n_contours;
	uint16_t	flags;
	uint8_t		cboxValid;
	uint8_t		reserved;
#if (90 == kPocoRotation) || (180 == kPocoRotation) || (270 == kPocoRotation)
	uint16_t	rw;
	uint16_t	rh;
#endif

	// CBox as integers (no fractional part)
	int16_t		xMin;
	int16_t		yMin;
	uint16_t	w;
	uint16_t	h;
	

	// points as FT_Vector_, contours as uint16_t, tags as uint8_t
};

typedef struct PocoOutlineRecord PocoOutlineRecord;
typedef struct PocoOutlineRecord *PocoOutline;

extern void PocoOutlineFill(Poco poco, PocoColor color, uint8_t blend, PocoOutline pOutline, PocoCoordinate dx, PocoCoordinate dy);

extern void PocoOutlineCalculateCBox(PocoOutline pOutline);
#if (90 == kPocoRotation) || (180 == kPocoRotation) || (270 == kPocoRotation)
extern void PocoOutlineRotate(PocoOutline pOutline, int w, int h);
extern void PocoOutlineUnrotate(PocoOutline pOutline);
#endif
