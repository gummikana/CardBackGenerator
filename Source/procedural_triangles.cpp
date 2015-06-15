#include "procedural_triangles.h"

#include <sdl.h>

#include <game_utils/tween/tween.h>
#include <game_utils/drawlines/drawlines.h>

#include <utils/color/color_utils.h>
#include <utils/math/cstatisticshelper.h>
#include <utils/vector_utils/vector_utils.h>
#include <utils/imagetoarray/imagetoarray.h>

#include "gameplay_utils/game_mouse.h"
#include "misc_utils/debug_layer.h"
#include "misc_utils/simple_profiler.h"
#include "misc_utils/file_dialog.h"

struct Triangle
{
	Triangle() : vert(3) { }

	std::vector< types::vector2 > vert;
	poro::types::fcolor color;
};

std::vector< Triangle > triangles;
std::vector< Uint32 > colors;

// --- colors -----

void LoadColors( const std::string& filename )
{
	colors.clear();
	using namespace imagetoarray;
	TempTexture* t = GetTexture( filename );
	
	for( int y = 0; y < t->height; ++y )
	{
		for( int x = 0; x < t->width; ++x )
		{
			Uint32 c = GetPixel( t, x, y );
			ceng::VectorAddUnique( colors, c );
		}
	}
}

poro::types::fcolor GetRandomColor( ceng::CLGMRandom* randomizer )
{
	int i = 0;
	if( randomizer && colors.empty() == false ) 
	{
		i = randomizer->Random( 0, (int)colors.size() - 1 );
	}
	else if( colors.empty() == false )
	{
		i = ceng::Random( 0, (int)colors.size() - 1 );
	}

	ceng::CColorFloat cf;
	cf.Set32( colors[i] );
	return poro::GetFColor( cf.GetB(), cf.GetG(), cf.GetR(), 1.f );
}

poro::types::fcolor FindClosestColor( ceng::CColorFloat o_color )
{
	float closest = 1000;
	int closest_i = 0;
	for( int i = 0; i < (int)colors.size(); ++i )
	{
		float dist = ceng::ColorDistance( o_color, ceng::CColorFloat( colors[i] ) );
		if( dist < closest )
		{
			closest = dist;
			closest_i = i;
		}
	}

	ceng::CColorFloat cf;
	cf.Set32( colors[closest_i] );
	return poro::GetFColor( cf.GetB(), cf.GetG(), cf.GetR(), 1.f );
}


// --- colors -----

#define CONFIG_TRIANGLE_LINES(list_) \
	list_(float,			height,					138.4f,			MetaData( 10.f, 512.f ) ) \
	list_(float,			width_percent,			0.9588f,		MetaData( 0.01f, 1.f ) ) \
	list_(bool,				offsetted,				true,			NULL ) \
	list_(float,			color_random,			0.1f,			MetaData( 0.f, 1.f ) ) \
	list_(bool,				non_random_colors,		true,			NULL ) \
	list_(float,			offset_x,				82.f,			MetaData( 0.f, 512.f ) ) \
	list_(float,			offset_y,				78.f,			MetaData( 0.f, 512.f ) ) \
	list_(float,			screen_width,			640.f,			MetaData( 0.f, 2048.f ) ) \
	list_(float,			screen_height,			1419,			MetaData( 0.f, 1596.f ) ) \
	list_(bool,				white_lines,			false,			NULL ) \
	list_(float,			line_width,				1.5f,			MetaData( 0.f, 5.f ) ) \
	list_(float,			line_alpha,				1.0f,			MetaData( 0.f, 1.f ) ) \
	list_(double,			seed,					1234,			MetaData( 0, 10000 ) ) \


DEFINE_CONFIG_UI( ConfigTriangle, CONFIG_TRIANGLE_LINES );
#undef CONFIG_TRIANGLE_LINES

ConfigTriangle config;


void TrianglesLine()
{
	triangles.clear();
	const float height = config.height;
	const float width = config.width_percent * height;
	const float SCREEN_WIDTH = 850;
	const float SCREEN_HEIGHT = 1125;

	ceng::CLGMRandom randomizer;
	randomizer.SetSeed( config.seed );

	bool random_colors = false;
	const float color_random = config.color_random;

	const int count_height = SCREEN_HEIGHT / height;
	const int count_width = SCREEN_WIDTH / width;

	const float color_width = config.screen_width / width;
	const float color_height = config.screen_height / height;


	for( int iy = 0; iy < count_height + 1; iy++ )
	{
		for( int ix = -1; ix < count_width + 1; ix++ )
		{
			float x = ((float)ix) * width;
			float y = ((float)iy) * height;

			if( config.offsetted && iy % 2 == 0 )
			{
				x += 0.5f * width;
			}

			// 2 at a time
			Triangle t;
			t.vert[0].Set( x, y + height );
			t.vert[1].Set( x + width * 0.5f, y );
			t.vert[2].Set( x + width, y + height );
			
			if( random_colors )
			{
				t.color = GetRandomColor( &randomizer );
			}
			else
			{
				types::fcolor fc;
				fc.r = ceng::math::Clamp( 1.f - ( (float)ix / (float)color_width ), 0.f, 1.f );
				fc.g = ceng::math::Clamp( ( (float)iy / (float)color_height ), 0.f, 1.f );
				fc.b = ceng::math::Clamp( ( (float)ix / (float)color_width ), 0.f, 1.f );
				
				fc.g *= ( 2.f + fc.r ) / 3.f;

				fc.r += randomizer( -color_random, color_random );
				fc.g += randomizer( -color_random, color_random );
				fc.b += randomizer( -color_random, color_random );
				t.color = FindClosestColor( fc );
			}
			// t.color = poro::GetFColor( randomizer( 0.f, 1.f ), randomizer( 0.f, 1.f ), randomizer( 0.f, 1.f ), 1.f ); 

			for( int i = 0; i < t.vert.size(); ++i ) 
			{
				t.vert[i].x += config.offset_x;
				t.vert[i].y += config.offset_y;
			}
			triangles.push_back( t );
 
			t.vert[0].Set( x + width * 0.5f, y );
			t.vert[1].Set( x + width, y + height );
			t.vert[2].Set( x + width * 1.5f, y );
			if( random_colors )
			{
				t.color = GetRandomColor( &randomizer );
			}
			else
			{
				types::fcolor fc;
				fc.r = ceng::math::Clamp( 1.f - ( (float)ix / (float)color_width ), 0.f, 1.f );
				fc.g = ceng::math::Clamp( ( (float)iy / (float)color_height ), 0.f, 1.f );
				fc.b = ceng::math::Clamp( ( (float)ix / (float)color_width ), 0.f, 1.f );

				fc.g *= ( 2.f + fc.r ) / 3.f;

				fc.r += randomizer( -color_random, color_random );
				fc.g += randomizer( -color_random, color_random );
				fc.b += randomizer( -color_random, color_random );
				t.color = FindClosestColor( fc );
			}

			// t.color = poro::GetFColor( randomizer( 0.f, 1.f ), randomizer( 0.f, 1.f ), randomizer( 0.f, 1.f ), 1.f ); 
			for( int i = 0; i < t.vert.size(); ++i ) 
			{
				t.vert[i].x += config.offset_x;
				t.vert[i].y += config.offset_y;
			}
			triangles.push_back( t );
		}
	}

}

// ----------------------------------------------------------------------------


#define CONFIG_ROOM(list_) \
	list_(float,			offset_x,				82.f,			MetaData( 0.f, 512.f ) ) \
	list_(float,			offset_y,				78.f,			MetaData( 0.f, 512.f ) ) \
	list_(float,			screen_width,			662.f,			MetaData( 1.f, 2048.f ) ) \
	list_(float,			screen_height,			968.f,			MetaData( 1.f, 1596.f ) ) \
	list_(float,			box_width_p,			0.077f,			MetaData( 0.f, 1.f ) ) \
	list_(float,			box_height_p,			0.23f,			MetaData( 0.f, 1.f ) ) \
	list_(float,			color_random,			0.1f,			MetaData( 0.f, 1.f ) ) \
	list_(int,				n_boxes,				5,				MetaData( 1, 20 ) ) \
	list_(int,				color_offset1,			0,				MetaData( 0, 10 ) ) \
	list_(int,				color_offset2,			1,				MetaData( 0, 10 ) ) \
	list_(int,				color_offset3,			2,				MetaData( 0, 10 ) ) \
	list_(int,				color_offset4,			3,				MetaData( 0, 10 ) ) \
	list_(bool,				normalized_boxes,		false,			NULL  ) \
	list_(bool,				white_lines,			false,			NULL ) \
	list_(float,			line_width,				1.5f,			MetaData( 0.f, 5.f ) ) \
	list_(float,			line_alpha,				1.0f,			MetaData( 0.f, 1.f ) ) \
	list_(double,			seed,					1234,			MetaData( 0, 10000 ) ) \


DEFINE_CONFIG_UI( ConfigRoom, CONFIG_ROOM );
#undef CONFIG_ROOM

ConfigRoom room_config;

void CycleColors( Triangle& t, int index, ceng::CLGMRandom* randomizer )
{
	if( colors.empty() ) return;

	if( randomizer ) index += randomizer->Random( 0, 3 );

	ceng::CColorFloat cf;
	cf.Set32( colors[index % colors.size()] );
	t.color = poro::GetFColor( cf.GetB(), cf.GetG(), cf.GetR(), 1.f );
}


void FindColorFor( Triangle& t, ceng::CLGMRandom* randomizer )
{
	types::vector2 pos;
	for( int i = 0; i < (int)t.vert.size(); ++i )
		pos += t.vert[i];

	pos.x /= (float)( t.vert.size() ); 
	pos.y /= (float)( t.vert.size() ); 

	pos.x /= room_config.screen_width;
	pos.y /= room_config.screen_height;

	types::fcolor fc;
	fc.r = ceng::math::Clamp( 1.f - pos.x, 0.f, 1.f );
	fc.g = ceng::math::Clamp( pos.y, 0.f, 1.f );
	fc.b = ceng::math::Clamp( pos.x, 0.f, 1.f );
	
	fc.g *= ( 2.f + fc.r ) / 3.f;

	if( randomizer )
	{
		fc.r += randomizer->Randomf( -room_config.color_random, room_config.color_random );
		fc.g += randomizer->Randomf( -room_config.color_random, room_config.color_random );
		fc.b += randomizer->Randomf( -room_config.color_random, room_config.color_random );
	}

	t.color = FindClosestColor( fc );
}

void AddTriangle( Triangle& t )
{
	for( std::size_t i = 0; i < t.vert.size(); ++i )
	{
		t.vert[i].x += room_config.offset_x;
		t.vert[i].y += room_config.offset_y;
	}

	triangles.push_back( t );
}



void TriangleRooms()
{
	triangles.clear();

	const types::vector2 offset( room_config.offset_x, room_config.offset_y );
	const types::vector2 size( room_config.screen_width, room_config.screen_height );

	const types::vector2 center_p = 0.5f * size;

	ceng::CLGMRandom randomizer;
	randomizer.SetSeed( room_config.seed );

	std::vector< types::vector2 > corners(4);

	// add the center box
	types::vector2 center_top = center_p;
	types::vector2 center_bottom = center_p;
	center_top.y -= room_config.box_height_p * size.y;
	center_bottom.y += room_config.box_height_p * size.y;
	
	{

		corners[0].Set( center_top.x - size.x * room_config.box_width_p, center_top.y ); 
		corners[1].Set( center_top.x + size.x * room_config.box_width_p, center_top.y ); 
		corners[2].Set( center_top.x + size.x * room_config.box_width_p, center_bottom.y ); 
		corners[3].Set( center_top.x - size.x * room_config.box_width_p, center_bottom.y ); 

		// left box
		Triangle left_box;
		left_box.vert.resize( 4 );
		left_box.vert[0].Set( center_top.x - size.x * room_config.box_width_p, center_top.y );
		left_box.vert[1].Set( center_top.x , center_top.y );
		left_box.vert[3].Set( center_top.x , center_bottom.y );
		left_box.vert[2].Set( center_top.x - size.x * room_config.box_width_p, center_bottom.y );

		left_box.color = GetRandomColor( &randomizer );
		CycleColors( left_box, 0, NULL );
		AddTriangle( left_box );

		Triangle right_box;
		right_box.vert.resize( 4 );
		right_box.vert[0].Set( center_top.x + size.x * room_config.box_width_p, center_top.y );
		right_box.vert[1].Set( center_top.x , center_top.y );
		right_box.vert[3].Set( center_top.x , center_bottom.y );
		right_box.vert[2].Set( center_top.x + size.x * room_config.box_width_p, center_bottom.y );

		right_box.color = GetRandomColor( &randomizer );
		CycleColors( right_box, 1, NULL );
		AddTriangle( right_box );

		// left triangle
		Triangle left;
		left.vert[0].Set( center_top.x - size.x * room_config.box_width_p, center_p.y );
		left.vert[1].Set( center_top.x , center_p.y - 0.5f * size.x * room_config.box_width_p );
		left.vert[2].Set( center_top.x , center_p.y + 0.5f * size.x * room_config.box_width_p );
		left.color = GetRandomColor( &randomizer );
		CycleColors( left, 1, NULL );
		AddTriangle( left );

		// left triangle
		Triangle right;
		right.vert[0].Set( center_top.x + size.x * room_config.box_width_p, center_p.y );
		right.vert[1].Set( center_top.x , center_p.y - 0.5f * size.x * room_config.box_width_p );
		right.vert[2].Set( center_top.x , center_p.y + 0.5f * size.x * room_config.box_width_p );
		right.color = GetRandomColor( &randomizer );
		CycleColors( right, 2, NULL );
		AddTriangle( right );
	}

	// generate the thing
	{
		using namespace ceng::math;


		const types::vector2 top_left( 0, 0 );
		const types::vector2 top_right( room_config.screen_width, 0 );
		const types::vector2 bottom_right( room_config.screen_width, room_config.screen_height );
		const types::vector2 bottom_left( 0, room_config.screen_height );

		for( int i = 0; i < room_config.n_boxes; ++i )
		{
			float low = ( (float)i / (float)room_config.n_boxes );
			float high = ( ((float)i+1) / (float)room_config.n_boxes );

			if( room_config.normalized_boxes )
			{
				low = 1.f - low;
				low = low * low;
				low = 1.f - low;

				high = 1.f - high;
				high = high * high;
				high = 1.f - high;
			}
		
			Triangle top_box;
			top_box.vert.resize( 4 );
			top_box.vert[0] = Lerp( top_left, corners[0], low );
			top_box.vert[1] = Lerp( top_right, corners[1], low );
			top_box.vert[2] = Lerp( top_left, corners[0], high );
			top_box.vert[3] = Lerp( top_right, corners[1], high );
			top_box.color = GetRandomColor( &randomizer );
			CycleColors( top_box, i + room_config.color_offset2, &randomizer );
			AddTriangle( top_box );

			
			Triangle right_box;
			right_box.vert.resize( 4 );
			right_box.vert[0] = Lerp( top_right, corners[1], low );
			right_box.vert[1] = Lerp( top_right, corners[1], high );
			right_box.vert[2]  = Lerp( bottom_right, corners[2], low );
			right_box.vert[3] = Lerp( bottom_right, corners[2], high );
			right_box.color = GetRandomColor( &randomizer );
			CycleColors( right_box, i + room_config.color_offset3, &randomizer );
			AddTriangle( right_box );
			

			Triangle bottom_box;
			bottom_box.vert.resize( 4 );
			bottom_box.vert[0] = Lerp( bottom_left, corners[3], low );
			bottom_box.vert[1] = Lerp( bottom_right, corners[2], low );
			bottom_box.vert[2] = Lerp( bottom_left, corners[3], high );
			bottom_box.vert[3] = Lerp( bottom_right, corners[2], high );
			bottom_box.color = GetRandomColor( &randomizer );
			CycleColors( bottom_box, i + room_config.color_offset4, &randomizer );
			AddTriangle( bottom_box );

			
			Triangle left_box;
			left_box.vert.resize( 4 );
			left_box.vert[0] = Lerp( top_left, corners[0], low );
			left_box.vert[1] = Lerp( top_left, corners[0], high );
			left_box.vert[2]  = Lerp( bottom_left, corners[3], low );
			left_box.vert[3] = Lerp( bottom_left, corners[3], high );
			left_box.color = GetRandomColor( &randomizer );
			CycleColors( left_box, i + room_config.color_offset1, &randomizer );
			AddTriangle( left_box );
		}
	}

}

// ----------------------------------------------------------------------------

#define CONFIG_STRIPES(list_) \
	list_(float,			offset_x,				82.f,			MetaData( 0.f, 512.f ) ) \
	list_(float,			offset_y,				78.f,			MetaData( 0.f, 512.f ) ) \
	list_(float,			screen_width,			662.f,			MetaData( 1.f, 2048.f ) ) \
	list_(float,			screen_height,			968.f,			MetaData( 1.f, 1596.f ) ) \
	list_(float,			scale_x,				1.f,			MetaData( 0.0001f, 3.f ) ) \
	list_(int,				stripe_count,			4,				MetaData( 0, 10 ) ) \
	list_(float,			stripe_l1,				100.f,			MetaData( 0.f, 1024.f ) ) \
	list_(float,			stripe_l2,				100.f,			MetaData( 0.f, 1024.f ) ) \
	list_(float,			stripe_l3,				100.f,			MetaData( 0.f, 1024.f ) ) \
	list_(float,			stripe_l4,				100.f,			MetaData( 0.f, 1024.f ) ) \
	list_(float,			stripe_l5,				100.f,			MetaData( 0.f, 1024.f ) ) \
	list_(float,			stripe_l6,				100.f,			MetaData( 0.f, 1024.f ) ) \
	list_(float,			stripe_l7,				100.f,			MetaData( 0.f, 1024.f ) ) \
	list_(float,			stripe_l8,				100.f,			MetaData( 0.f, 1024.f ) ) \
	list_(float,			stripe_l9,				100.f,			MetaData( 0.f, 1024.f ) ) \
	list_(int,				stripe_c1,				0,				MetaData( 0, 10 ) ) \
	list_(int,				stripe_c2,				1,				MetaData( 0, 10 ) ) \
	list_(int,				stripe_c3,				2,				MetaData( 0, 10 ) ) \
	list_(int,				stripe_c4,				3,				MetaData( 0, 10 ) ) \
	list_(int,				stripe_c5,				4,				MetaData( 0, 10 ) ) \
	list_(int,				stripe_c6,				5,				MetaData( 0, 10 ) ) \
	list_(int,				stripe_c7,				6,				MetaData( 0, 10 ) ) \
	list_(int,				stripe_c8,				7,				MetaData( 0, 10 ) ) \
	list_(int,				stripe_c9,				8,				MetaData( 0, 10 ) ) \
	list_(double,			seed,					1234,			MetaData( 0, 10000 ) ) \


DEFINE_CONFIG_UI( ConfigStripes, CONFIG_STRIPES );
#undef CONFIG_STRIPES

ConfigStripes stripes_config;

void DoStripes()
{

	triangles.clear();

	const types::vector2 offset( stripes_config.offset_x, stripes_config.offset_y );
	const types::vector2 size( stripes_config.screen_width, stripes_config.screen_height );

	ceng::CLGMRandom randomizer;
	randomizer.SetSeed( stripes_config.seed );

	std::vector< float > lengths(10);
	lengths[1] = stripes_config.stripe_l1;
	lengths[2] = stripes_config.stripe_l2;
	lengths[3] = stripes_config.stripe_l3;
	lengths[4] = stripes_config.stripe_l4;
	lengths[5] = stripes_config.stripe_l5;
	lengths[6] = stripes_config.stripe_l6;
	lengths[7] = stripes_config.stripe_l7;
	lengths[8] = stripes_config.stripe_l8;
	lengths[9] = stripes_config.stripe_l9;

	std::vector< int > colors(10);
	colors[1] = stripes_config.stripe_c1;
	colors[2] = stripes_config.stripe_c2;
	colors[3] = stripes_config.stripe_c3;
	colors[4] = stripes_config.stripe_c4;
	colors[5] = stripes_config.stripe_c5;
	colors[6] = stripes_config.stripe_c6;
	colors[7] = stripes_config.stripe_c7;
	colors[8] = stripes_config.stripe_c8;
	colors[9] = stripes_config.stripe_c9;

	// add the center box
	float pos_x = 0;
	while( pos_x < stripes_config.screen_width )
	{
		for( int i = 1; i <= stripes_config.stripe_count; ++i )
		{
			float width = lengths[i];
			int color = colors[i];

			width *= stripes_config.scale_x;

			// left box
			Triangle box;
			box.vert.resize( 4 );
			box.vert[0].Set( pos_x, 0 ); 
			box.vert[1].Set( pos_x + width, 0 ); 
			box.vert[2].Set( pos_x, stripes_config.screen_height ); 
			box.vert[3].Set( pos_x + width, stripes_config.screen_height ); 

			CycleColors( box, color, NULL );
			AddTriangle( box );

			pos_x += width;

			if( pos_x >= stripes_config.screen_width ) 
				break;
		}
	}
}

// ----------------------------------------------------------------------------


void DrawTriangle( poro::IGraphics* graphics, const Triangle& t )
{
	static std::vector< poro::types::vec2 > poro_vertices(4);

	poro_vertices.resize( t.vert.size() );
	for( int i = 0; i < (int)t.vert.size(); ++i )
	{
		poro_vertices[i].x = t.vert[i].x;
		poro_vertices[i].y = t.vert[i].y;
	}

	graphics->SetDrawFillMode(1);
	graphics->DrawFill( poro_vertices, t.color );
}

// ----------------------------------------------------------------------------


ProceduralTriangles::ProceduralTriangles()
{
}


void ProceduralTriangles::Exit()
{
	mDebugLayer.reset( NULL );
}

//-----------------------------------------------------------------------------

void ProceduralTriangles::Init()
{
	DefaultApplication::Init();
	mDebugLayer.reset( new DebugLayer );
	// Poro()->GetGraphics()->SetFillColor( poro::GetFColor( 248.f / 255.f, 245.f / 255.f, 236.f / 255.f, 1.f ) );
	// Poro()->SetPrintFramerate( false );

	GameMouse::GetSingletonPtr()->Init();

	mOverlay = as::LoadSprite( "data/overlay.png" );

	mDebugLayer->OpenConfig( stripes_config );
	LoadColors( "data/colors/gradientish.png" );
}

// ----------------------------------------------------------------------------

void ProceduralTriangles::Clear()
{
}

// ----------------------------------------------------------------------------

void ProceduralTriangles::Update( float dt )
{
	UpdateGTweens( dt );

	if( mDebugLayer.get() ) 
		mDebugLayer->Update( dt );

	// MouseButtonDown(poro::types::vec2(), 1);

	// TriangleRooms();
	// TrianglesLine();
	DoStripes();

	GameMouse::GetSingletonPtr()->OnFrameEnd();

	/*if( mStarted && ( Poro()->GetTime() - mStartTime ) < 5.f && ceng::Random( 0, 100 ) < 8 )
		OnMouseDown();*/

}

// ----------------------------------------------------------------------------

void ProceduralTriangles::Draw( poro::IGraphics* graphics )
{ 
	for( std::size_t i = 0; i < triangles.size(); ++i )
	{
		DrawTriangle( graphics, triangles[i] );
	}

	if( room_config.white_lines )
	{
		poro::types::fcolor color = poro::GetFColor( 1.f,1.f,1.f, room_config.line_alpha ); 
		SetLineWidth( room_config.line_width );
		for( std::size_t i = 0; i < triangles.size(); ++i )
		{
			for( int j = 0; j < triangles[i].vert.size(); ++j )
			{
				DrawLine( graphics, triangles[i].vert[j], triangles[i].vert[ ( j + 1 ) % triangles[i].vert.size() ], color );
			}
		}
	}

	as::DrawSprite( mOverlay, graphics );

	if( mDebugLayer.get() ) 
		mDebugLayer->Draw( graphics );

}

// ----------------------------------------------------------------------------

void ProceduralTriangles::MouseMove(const poro::types::vec2& p)
{
	if (mDebugLayer.get())
		mDebugLayer->mMousePositionInWorld = types::vector2(p);
}

void ProceduralTriangles::MouseButtonDown(const poro::types::vec2& p, int button)
{
	
}

void ProceduralTriangles::MouseButtonUp(const poro::types::vec2& pos, int button)
{
}

//=============================================================================

void ProceduralTriangles::OnKeyDown( int key, poro::types::charset unicode )
{
	if( key == 27 ) 
		Poro()->Exit();

	if( key == SDLK_o && Poro()->GetKeyboard()->IsCtrlDown() )
	{
		std::string im_file = LoadFileDialog( "data/colors/" );
		if( im_file.empty() == false )
		{
			LoadColors( im_file );
		}
	}

	if( key == SDLK_r )
	{
		int color_base = ceng::Random( 0, 256 );
		stripes_config.stripe_c1 = color_base + ceng::Random( 0, 10 );
		stripes_config.stripe_c2 = color_base + ceng::Random( 0, 10 );
		stripes_config.stripe_c3 = color_base + ceng::Random( 0, 10 );
		stripes_config.stripe_c4 = color_base + ceng::Random( 0, 10 );
		stripes_config.stripe_c5 = color_base + ceng::Random( 0, 10 );
		stripes_config.stripe_c6 = color_base + ceng::Random( 0, 10 );
		stripes_config.stripe_c7 = color_base + ceng::Random( 0, 10 );
		stripes_config.stripe_c8 = color_base + ceng::Random( 0, 10 );
		stripes_config.stripe_c9 = color_base + ceng::Random( 0, 10 );

		stripes_config.stripe_l1 = ceng::Randomf( 10.f, 200.f );
		stripes_config.stripe_l2 = ceng::Randomf( 10.f, 200.f );
		stripes_config.stripe_l3 = ceng::Randomf( 10.f, 200.f );
		stripes_config.stripe_l4 = ceng::Randomf( 10.f, 200.f );
		stripes_config.stripe_l5 = ceng::Randomf( 10.f, 200.f );
		stripes_config.stripe_l6 = ceng::Randomf( 10.f, 200.f );
		stripes_config.stripe_l7 = ceng::Randomf( 10.f, 200.f );
		stripes_config.stripe_l8 = ceng::Randomf( 10.f, 200.f );
		stripes_config.stripe_l9 = ceng::Randomf( 10.f, 200.f );

		stripes_config.stripe_count = ceng::Random( 1, 9 );
	}

}

//-----------------------------------------------------------------------------


void ProceduralTriangles::OnKeyUp( int key, poro::types::charset unicode )
{
}

//=============================================================================

