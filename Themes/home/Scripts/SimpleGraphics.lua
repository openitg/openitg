--[[
OpenITG simple graphics configuration, version 0.2
Licensed under Creative Commons Attribution-Share Alike 3.0 Unported
(http://creativecommons.org/licenses/by-sa/3.0/)

These probably won't work unless they're used on the same screen. You've been warned.
Written by Mark Cannon ("Vyhd") for OpenITG (http://www.boxorroxors.net/)
All I ask is that you keep this notice intact and don't redistribute in bytecode.
--]]

-- aliases for the different config levels
local GraphicsLevels = { "LOW", "MEDIUM", "HIGH", "MAXIMUM" }

-- this is a little oddly written, in order to facilitate options setting.
-- each key is named after the preference it's intended to set, and has a
-- table with values corresponding to the above levels. that way, we can
-- simply do k,v for pairs() and refer to v[level]
local GraphicsSettings =
{
	DisplayColorDepth		= {	16,	32,	32,	32	},
	TextureColorDepth		= {	16,	32,	32,	32	},
	MovieColorDepth			= {	16,	16,	16,	32	},
	MaxTextureResolution		= {	256,	512,	1024,	2048	},

	DelayedTextureDelete		= {	false,	false,	false,	true	},
	DelayedModelDelete		= {	false,	false,	true,	true	},
	PalettedBannerCache		= {	false,	false,	true,	true	},

	TrilinearFiltering		= {	false,	false,	true,	true	},
	AnisotropicFiltering		= {	false,	false,	true,	true	},

	BannerCache			= {	0,	0,	1,	1	},

	-- XXX: this is here so we have some way of setting it...
	ThreadedMovieDecode		= {	false,	false,	false,	true	},
	SmoothLines			= {	false,	true,	true,	true	},
}

function LuaGraphicOptions()
	local function Load(self, list, pn)
		local cur_res = PREFSMAN:GetPreference("MaxTextureResolution")
		for i=1,table.getn(GraphicsLevels) do
			if cur_res == GraphicsSettings.MaxTextureResolution[i] then list[i] = true return end
		end

		-- default to "MEDIUM"
		list[2] = true;
	end

	local function Save(self, list, pn)
		for i=1,table.getn(GraphicsLevels) do
			if list[i] == true then
				for pref,tbl in pairs(GraphicsSettings) do
					PREFSMAN:SetPreference( pref, tbl[i] )
				end

				GAMESTATE:DelayedGameCommand( "reloadtheme" )
				return
			end
		end
	end

	local Params = { Name = "GraphicQuality" }

	return CreateOptionRow( Params, GraphicsLevels, Load, Save )
end
