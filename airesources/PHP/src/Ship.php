<?php

class Ship extends Entity
{
    const UNDOCK_KEY = 'u';
    const DOCK_KEY = 'd';
    const THRUST_KEY = 't';

    /**
     * @var Velocity
     */
    private $velocity;

    /**
     * @var bool
     */
    private $docked;

    /**
     * @var int
     */
    private $planetId;

    /**
     * @var float
     */
    private $dockingProgress;

    /**
     * @var float
     */
    private $weaponCooldown;

    /**
     * @var Planet|null
     */
    private $planet;

    /**
     * @var Coordinate
     */
    private $coordinateNextTurn;


    public function __construct(
        Player $owner,
        int $id,
        Coordinate $coordinate,
        int $health,
        Velocity $velocity,
        bool $docked,
        int $planetId,
        float $dockingProgress,
        float $weaponCooldown
    ) {
        parent::__construct($owner, $id, $coordinate, $health, 0.5);
        $this->velocity = $velocity;
        $this->docked = $docked;
        $this->planetId = $planetId;
        $this->dockingProgress = $dockingProgress;
        $this->weaponCooldown = $weaponCooldown;
    }

    public function getVelocity(): Velocity
    {
        return $this->velocity;
    }

    public function isDocked(): bool
    {
        return $this->docked;
    }

    public function getPlanetId(): int
    {
        return $this->planetId;
    }

    public function getDockingProgress(): float
    {
        return $this->dockingProgress;
    }

    public function getWeaponCooldown(): float
    {
        return $this->weaponCooldown;
    }

    public function thrust(int $thrust, float $angle): string
    {
        return self::THRUST_KEY.' '.$this->getId().' '.$thrust.' '.round($angle);
    }

    public function dock(int $planetId): string
    {
        return self::DOCK_KEY.' '.$this->getId().' '.$planetId;
    }

    public function undock(): string
    {
        return self::UNDOCK_KEY.' '.$this->getId();
    }

    public function canDock(Planet $planet): bool
    {
        return $planet->isDockable($this->getOwner()) &&
            $planet->getCoordinate()->getDistanceTo($this->getCoordinate()) <= $planet->getRadius() + 4;
    }

    public function navigate(
        Map $map,
        Coordinate $target,
        int $thrust,
        bool $avoidObstacles,
        int $maxCorrections,
        float $angularStepRad,
        Logger $logger
    ): string {
        $logger->log(
            'Navigate from '.json_encode($this->getCoordinate()).' to '.json_encode($target).' / max corrections '.$maxCorrections
        );

        if ($maxCorrections <= 0) {
            return '';
        }
        $distance = $this->getCoordinate()->getDistanceTo($target);
        $angleRad = $this->getCoordinate()->getAngleTo($target);

        $obstacles =  $map->getEntitiesBetween($this, $target);
        $obstacles = iterator_to_array($obstacles);
        if ($avoidObstacles && $obstacles) {
            $newTargetDx = cos($angleRad + $angularStepRad) * $distance;
            $newTargetDy = sin($angleRad + $angularStepRad) * $distance;
            $newTarget = new Coordinate($this->getCoordinate()->getX() + $newTargetDx, $this->getCoordinate()->getY() + $newTargetDy);

            $logger->log('Has Obstacles, correct position to: '.json_encode($newTarget));

            return $this->navigate($map, $newTarget, $thrust, $avoidObstacles, $maxCorrections - 1, $angularStepRad, $logger);
        }

        $computedThrust = $thrust;
        if ($distance < $thrust) {
            $computedThrust = (int) $distance;
        }

        $angleDeg = self::angleRadToDegClipped($angleRad);
        $logger->log('Distance: '.$distance.' / AngleRad: '.$angleRad.' / AngleDeg: '.$angleDeg.' / Thrust '.$computedThrust);
        $this->coordinateNextTurn = $target->forecastMove($computedThrust, $angleDeg);
        return $this->thrust($computedThrust, $angleDeg);
    }

    public function getCoordinateNextTurn(): ?Coordinate
    {
        return $this->coordinateNextTurn;
    }

    public function getPlanet(): ?\Planet
    {
        return $this->planet;
    }

    public function setPlanet(?Planet $planet): void
    {
        $this->planet = $planet;
    }

    private static function angleRadToDegClipped(float $angleRad): int
    {
        $degUnclipped = round(rad2deg($angleRad));

        return (int) ((($degUnclipped % 360) + 360) % 360);
    }

    public function jsonSerialize(): array
    {
        return array_merge(
            parent::jsonSerialize(),
            [
                'velocity' => $this->getVelocity()->jsonSerialize(),
                'docked' => $this->isDocked() ? 'yes' : 'no',
                'planetId' => $this->getPlanetId(),
                'dockingProgress' => $this->getDockingProgress(),
                'weaponCooldown' => $this->getWeaponCooldown(),
            ]
        );
    }


}
