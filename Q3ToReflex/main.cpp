//
//	Author:	Michael Cameron
//	Email:	chronokun@hotmail.com
//

// Libraries Include
#include "libraries.h"
// Local Includes
#include "constants.h"
#include "mathtypes.h"
#include "vector.h"
#include "geometry.h"
#include "brush.h"
#include "q3mapparser.h"

const std::string& GetBrushString(std::string& _rOutput, const TPolyBrush& _krBrush, const bool _kbNoClip, const bool _kbNoTrigger, const bool _kbAllCaulk)
{
	// Cull any brushes we don't wish to import, eg. visibility hints etc.
	bool bCullBrush = false;
	for(const TPolyBrushFace& krFace : _krBrush.m_Faces)
	{
		if(CheckForBrushCull(krFace.m_Material))
		{
			bCullBrush = true;
		}
		if(_kbNoTrigger && strcmp(krFace.m_Material.c_str(), "common/trigger") == 0)
		{
			bCullBrush = true;
		}
		if(_kbNoClip && strcmp(krFace.m_Material.c_str(), "internal/editor/textures/editor_clip") == 0)
		{
			bCullBrush = true;
		}
	}

	// Check brush wasn't culled and has a valid number of faces
	if(_krBrush.m_Faces.size() >= 4 && !bCullBrush)
	{
		std::stringstream ssOutput;

		ssOutput << std::fixed;
		ssOutput << "\tbrush" << std::endl;
		ssOutput << "\t\tvertices" << std::endl;
		for(size_t i = 0; i < _krBrush.m_Vertices.size(); ++i)
		{
			// Coordinate space needs to be transformed from q3 coordinate space to reflex coordinate space (RightHanded +Z Up -> LeftHanded +Y Up)
			ssOutput << "\t\t\t" << _krBrush.m_Vertices[i].m_dX << " " << _krBrush.m_Vertices[i].m_dZ << " " << _krBrush.m_Vertices[i].m_dY << " " << std::endl;
		}
		ssOutput << "\t\tfaces" << std::endl;
		for(size_t i = 0; i < _krBrush.m_Faces.size(); ++i)
		{
			if(_krBrush.m_Faces[i].m_Indices.size() > 2)
			{
				ssOutput << "\t\t\t" << _krBrush.m_Faces[i].m_dTexCoordU << " "
					<< _krBrush.m_Faces[i].m_dTexCoordV << " "
					<< _krBrush.m_Faces[i].m_dTexScaleU * 2.0 << " "
					<< _krBrush.m_Faces[i].m_dTexScaleV * 2.0 << " "
					<< _krBrush.m_Faces[i].m_dTexRotation;
				for(size_t j = 0; j < _krBrush.m_Faces[i].m_Indices.size(); ++j)
				{
					ssOutput << " " << _krBrush.m_Faces[i].m_Indices[j];
				}
				if(		(!_kbAllCaulk)
					||	(strcmp(_krBrush.m_Faces[i].m_Material.c_str(), "common/trigger") == 0)
					||	(strcmp(_krBrush.m_Faces[i].m_Material.c_str(), "internal/editor/textures/editor_clip") == 0))
				{
					ssOutput << " " << _krBrush.m_Faces[i].m_Material << std::endl;
				}
				else
				{
					ssOutput << " " << "internal/editor/textures/editor_nolight" << std::endl;
				}
			}
		}

		_rOutput = ssOutput.str();
	}
	return(_rOutput);
}

int main(const int _kiArgC, const char** _kppcArgv)
{
	bool bNoPatch = false;
	bool bNoClip = false;
	bool bNoTrigger = false;
	bool bAllCaulk = false;
	int iTessFactor = 3;

	// Check we have correct number of parameters
	if(_kiArgC < 3)
	{
		return(3);
	}

	// Parse input file
	CQ3MapParser Parser;
	const bool kbSuccess = Parser.ParseQ3Map(_kppcArgv[1]);

	if(!kbSuccess)
	{
		return(1);
	}
	else
	{
		// Open output file
		std::ofstream OutFile;
		OutFile.open(_kppcArgv[2]);
		if(!OutFile.is_open())
		{
			return(2);
		}

		// Check commandline switches
		for(int i = 3; i < _kiArgC; ++i)
		{
			if(strcmp("-nopatches", _kppcArgv[i]) == 0)
			{
				bNoPatch = true;
			}
			else if(strcmp("-noclip", _kppcArgv[i]) == 0)
			{
				bNoClip = true;
			}
			else if(strcmp("-notrigger", _kppcArgv[i]) == 0)
			{
				bNoTrigger = true;
			}
			else if(strcmp("-allcaulk", _kppcArgv[i]) == 0)
			{
				bAllCaulk = true;
			}
			else if(strcmp("-tessfactor", _kppcArgv[i]) == 0)
			{
				if(i+1 < _kiArgC && isdigit((int)_kppcArgv[i+1][0]))
				{
					int iTemp = atoi(_kppcArgv[i+1]);
					if(iTemp >= 1 && iTemp <= 7)
					{
						iTessFactor = iTemp;
					}
				}
			}
		}

		std::vector<TPolyBrush> PolyBrushes(Parser.m_Brushes.size());

		// For all quake style plane brushes get reflex style polygon brush equivalent
		for(size_t i = 0; i < Parser.m_Brushes.size(); ++i)
		{
			PolyBrushes[i] = ToPolyBrush(TPolyBrush(), Parser.m_Brushes[i]);
		}


		std::vector<TPolyBrush> PatchBrushes;
		for(size_t i = 0; i < Parser.m_PatchDefs.size(); ++i)
		{
			if(		(Parser.m_PatchDefs[i].m_ControlPoints.size() > 0)
				&&	(Parser.m_PatchDefs[i].m_ControlPoints[0].size() == Parser.m_PatchDefs[i].m_szRows)
				&&	(Parser.m_PatchDefs[i].m_ControlPoints.size() == Parser.m_PatchDefs[i].m_szColumns))
			{
				TVectorD3 Controls[9];
				TVectorD3 ControlsM[3][3];
				for(size_t y = 0; y < (Parser.m_PatchDefs[i].m_szRows-2); y++)
				{
					for(size_t x = 0; x < (Parser.m_PatchDefs[i].m_szColumns-2); x++)
					{
						// Controls
						ControlsM[0][0] = Parser.m_PatchDefs[i].m_ControlPoints[x+0][y+0];
						ControlsM[0][1] = Parser.m_PatchDefs[i].m_ControlPoints[x+0][y+1];
						ControlsM[0][2] = Parser.m_PatchDefs[i].m_ControlPoints[x+0][y+2];
						
						ControlsM[1][0] = Parser.m_PatchDefs[i].m_ControlPoints[x+1][y+0];
						ControlsM[1][1] = Parser.m_PatchDefs[i].m_ControlPoints[x+1][y+1];
						ControlsM[1][2] = Parser.m_PatchDefs[i].m_ControlPoints[x+1][y+2];
						
						ControlsM[2][0] = Parser.m_PatchDefs[i].m_ControlPoints[x+2][y+0];
						ControlsM[2][1] = Parser.m_PatchDefs[i].m_ControlPoints[x+2][y+1];
						ControlsM[2][2] = Parser.m_PatchDefs[i].m_ControlPoints[x+2][y+2];

						for(int j = 0; j < 9; ++j)
						{
							Controls[j] = ControlsM[j/3][j%3];
							//Controls[j] = Parser.m_PatchDefs[i].m_ControlPoints[j/3][j%3];
						}

						const std::vector<TPolyBrush> kBrushes = BuildPatchBrushes(std::vector<TPolyBrush>(), Controls, iTessFactor, Parser.m_PatchDefs[i].m_Material, 0.0, 0.0, 1.0, 1.0, 0.0);
						for(size_t k = 0; k < kBrushes.size(); ++k)
						{
							PatchBrushes.push_back(kBrushes[k]);
						}
					}
				}
			}
		}

		// Add map header and WorldSpawn entity
		OutFile << "reflex map version 8\n"
				<< "global\n"
				<< "\tentity\n"
				<< "\t\ttype WorldSpawn\n";
		// Add brushes
		for(size_t i = 0; i < PolyBrushes.size(); ++i)
		{
			const std::string BrushString = GetBrushString(std::string(), PolyBrushes[i], bNoClip, bNoTrigger, bAllCaulk);
			OutFile << BrushString;
		}
		// Add patches
		if(!bNoPatch)
		{
			for(size_t i = 0; i < PatchBrushes.size(); ++i)
			{
				const std::string PatchString = GetBrushString(std::string(), PatchBrushes[i], bNoClip, bNoTrigger, bAllCaulk);
				OutFile << PatchString;
			}
		}
		for (size_t i = 0; i < Parser.m_Entities.size(); i++)
		{
			if (Parser.m_Entities[i].m_Classname == "info_player_deathmatch" || 
				Parser.m_Entities[i].m_Classname == "info_player_start" ||
				Parser.m_Entities[i].m_Classname == "info_player_start2" ||
				Parser.m_Entities[i].m_Classname == "info_player_coop")
			{
				auto origin = Parser.m_Entities[i].m_Properties.find("origin");
				auto angle = Parser.m_Entities[i].m_Properties.find("angle");
				auto spawnflags = Parser.m_Entities[i].m_Properties.find("spawnflags");
				double oldAngle = 0;
				size_t offset;
				std::string angleStr = "", spawnStr = "", originStr = "";

				double x = 0.0f, y = 0.0f, z = 0.0f;

				if (origin != Parser.m_Entities[i].m_Properties.end() && origin->second.size() > 0) //Origin found
				{
					const char *originTemp = origin->second.c_str();
					x = std::stod(originTemp, &offset);
					originTemp += offset;
					y = std::stod(originTemp, &offset);
					originTemp += offset;
					z = std::stod(originTemp, &offset);
				}

				originStr = "\t\tVector3 position " + std::to_string(x) + " " + std::to_string(z) + " " + std::to_string(y) + "\n";
				if (angle != Parser.m_Entities[i].m_Properties.end() && angle->second.size() > 0)
					oldAngle = std::stod(angle->second.c_str());
				angleStr = "\t\tVector3 angles " + std::to_string(oldAngle + 90.0f) + " " + std::to_string(0.0000f) + " " + std::to_string(0.0000f) + "\n"; //beautiful


				if (spawnflags != Parser.m_Entities[i].m_Properties.end() && spawnflags->second.size() > 0)
				{
					int spawnfl = 0;
					spawnfl = std::stoi(spawnflags->second.c_str());
					if (spawnfl & 1)
						spawnStr += "\t\tBool8 TeamA 0\n"; //idk why but it has to be 0
					if (spawnfl & 2)
						spawnStr += "\t\tBool8 TeamB 0\n";
				}

				OutFile << "\tentity\n"
					<< "\t\ttype PlayerSpawn\n";
				OutFile << originStr;
				OutFile << angleStr;
				OutFile << spawnStr;
				
				
			}
		}
		// Close output file
		OutFile.close();
	}
	// Return success
	return(0);
}