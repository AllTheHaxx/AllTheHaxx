Import("twdata")

function TuneGetCurvature(Weapon)
	local Curvature = 0
	if (Weapon == WEAPON_GUN) then
		Curvature = Game.Tuning().GunCurvature.Value
	end
	if (Weapon == WEAPON_SHOTGUN) then
		Curvature = Game.Tuning().ShotgunCurvature.Value
	end
	if (Weapon == WEAPON_GRENADE) then
		Curvature = Game.Tuning().GrenadeCurvature.Value
	end
	return Curvature/100
end

function TuneGetSpeed(Weapon)
	local Speed = 0
	if (Weapon == WEAPON_GUN) then
		Speed = Game.Tuning().gun_speed.Value
	end
	if (Weapon == WEAPON_SHOTGUN) then
		Speed = Game.Tuning().shotgun_speed.Value
	end
	if (Weapon == WEAPON_GRENADE) then
		Speed = Game.Tuning().grenade_speed.Value
	end
	return Speed/100
end

function TuneGetLifetime(Weapon)
	local Time = 1000
	if (Weapon == WEAPON_GUN) then
		Time = Game.Tuning().gun_lifetime.Value
	end
	if (Weapon == WEAPON_SHOTGUN) then
		Time = Game.Tuning().shotgun_lifetime.Value
	end
	if (Weapon == WEAPON_GRENADE) then
		Time = Game.Tuning().grenade_lifetime.Value
	end
	return Time/100
end
